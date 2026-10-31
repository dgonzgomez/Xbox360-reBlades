#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "blades.h"
#include "ui_audio_xsdk.h"
#include "blades_shaders.h"
#include "xsdk/gpu.h"
#include "xsdk/kernel.h"
#include "xsdk/log.h"
#include "xsdk/present.h"
#include "xsdk/shader.h"
#include "xsdk/texture.h"
#include "xsdk/xam.h"
#include "screen_layout.h"
#include "stick.h"
#include "library.h"
#include <time.h>

enum {
	BUFFERS = 2,
	SWAP_INTERVAL = 1,
	BEGIN_TIMEOUT_MS = 5000,
	VERTICES = 4,
	INDICES = 6,
	VERTICES_BYTES = VERTICES * STRIDE_BYTES,
	INDICES_BYTES = INDICES * 2,
};

static void write_gpu(volatile uint32_t *memory, const void *data, uint32_t size_bytes)
{
	const uint32_t words = size_bytes / 4;
	uint32_t i;
	for (i = 0; i < words; i++) {
		uint32_t word;
		memcpy(&word, (const uint8_t *)data + i * 4, 4);
		memory[i] = word;
	}
	xsdk_gpu_invalidate(xsdk_gpu_address(memory), size_bytes);
}

static bool make_mesh(volatile uint32_t **vertices, volatile uint32_t **indices)
{
	const float verts[VERTICES][VERTEX_FLOATS] = {
		{ -1.0f, 1.0f, 0.0f, 0.0f },
		{ 1.0f, 1.0f, 1.0f, 0.0f },
		{ -1.0f, -1.0f, 0.0f, 1.0f },
		{ 1.0f, -1.0f, 1.0f, 1.0f },
	};
	const uint16_t index_values[INDICES] = { 0, 1, 2, 2, 1, 3 };
	*vertices = xsdk_gpu_alloc(VERTICES_BYTES);
	*indices = xsdk_gpu_alloc(INDICES_BYTES);
	if (!*vertices || !*indices)
		return false;
	write_gpu(*vertices, verts, VERTICES_BYTES);
	write_gpu(*indices, index_values, INDICES_BYTES);
	return true;
}

static uint32_t read_pad(BladesHeld *held)
{
	static BladesStick stick;
	INPUT_STATE input = { 0 };
	uint32_t buttons = 0;
	memset(held, 0, sizeof *held);
	if (XamInputGetState(0, 0, &input) != ERROR_SUCCESS) {
		stick.direction = 0;
		return 0;
	}
	buttons = input.gamepad.buttons;
	held->up = (buttons & INPUT_GAMEPAD_DPAD_UP) != 0;
	held->down = (buttons & INPUT_GAMEPAD_DPAD_DOWN) != 0;
	held->left = (buttons & INPUT_GAMEPAD_DPAD_LEFT) != 0;
	held->right = (buttons & INPUT_GAMEPAD_DPAD_RIGHT) != 0;
	held->a = (buttons & INPUT_GAMEPAD_A) != 0;
	held->b = (buttons & INPUT_GAMEPAD_B) != 0;
	held->y = (buttons & INPUT_GAMEPAD_Y) != 0;
	held->x = (buttons & INPUT_GAMEPAD_X) != 0;
	held->guide = (buttons & INPUT_GAMEPAD_START) != 0;
	blades_stick_apply(held, &stick, input.gamepad.thumb_lx, input.gamepad.thumb_ly);
	return buttons;
}

static uint64_t microseconds(void)
{
	struct timespec stamp;
	if(clock_gettime(CLOCK_MONOTONIC,&stamp)!=0)return 0;
	return (uint64_t)stamp.tv_sec*1000000+(uint64_t)stamp.tv_nsec/1000;
}

int main(void)
{
	XsdkShader vertex, pixel;
	XsdkTexture texture;
	XsdkSamplerState sampler_state;
	XsdkDepthStencilState depth_state;
	volatile uint32_t *vertices = 0;
	volatile uint32_t *indices = 0;
	uint32_t width = 0;
	uint32_t height = 0;
	NTSTATUS status;
	const XsdkSampler sampler = { .mag_filter = XSDK_FILTER_POINT,
		.min_filter = XSDK_FILTER_POINT,
		.mip_filter = XSDK_FILTER_POINT,
		.address_u = XSDK_ADDRESS_CLAMP,
		.address_v = XSDK_ADDRESS_CLAMP };
	const XsdkDepthStencil depth = { .depth_test = false, .depth_write = false };

	xsdk_log_write("blades build v5: GOD library discovery and signed-in dashboard profile");
	xsdk_log_write("blades startup 1: decoding assets and painting initial CPU frame");
	blades_init();
	static BladesGame installed[BLADES_MAX_GAMES];
	int installed_count=library_scan("hdd:/Content/0000000000000000",installed,BLADES_MAX_GAMES);
	xsdk_log_write("blades: GOD scan hdd:/Content/0000000000000000 returned %d",installed_count);
	blades_set_library(installed,installed_count);
	xsdk_log_write("blades startup 2: CPU frame ready; opening GPU");
	status = xsdk_gpu_open();
	if (!NT_SUCCESS(status)) {
		xsdk_log_write("blades: xsdk_gpu_open failed: 0x%08X", (unsigned)status);
		return 1;
	}
	xsdk_log_write("blades startup 3: GPU open; loading shaders/layout/mesh");
	status = xsdk_shader_load(&vertex, screen_vert.container, screen_vert.container_bytes);
	if (NT_SUCCESS(status))
		status = xsdk_shader_load(&pixel, screen_frag.container, screen_frag.container_bytes);
	if (NT_SUCCESS(status))
		status = xsdk_shader_set_vertex_layout(&vertex, layout, sizeof layout / sizeof layout[0],
			(const uint32_t[]){ STRIDE_BYTES });
	if (!NT_SUCCESS(status) || !make_mesh(&vertices, &indices)) {
		xsdk_log_write("blades: setup failed: 0x%08X", (unsigned)status);
		return 1;
	}
	xsdk_log_write("blades startup 4: shaders/mesh ready; allocating framebuffer texture");
	status = xsdk_texture_create(&texture, XSDK_FORMAT_A8R8G8B8, BLADES_WIDTH, BLADES_HEIGHT, 1, true);
	if (!NT_SUCCESS(status)) {
		xsdk_log_write("blades: xsdk_texture_create failed: 0x%08X", (unsigned)status);
		return 1;
	}
	xsdk_texture_sampler_state(&sampler_state, &sampler);
	xsdk_gpu_depth_stencil_state(&depth_state, &depth);
	xsdk_log_write("blades startup 5: texture ready; opening display");
	status = xsdk_present_open(XSDK_DISPLAY_720P, BUFFERS, SWAP_INTERVAL, &width, &height);
	if (!NT_SUCCESS(status)) {
		xsdk_log_write("blades: xsdk_present_open failed: 0x%08X", (unsigned)status);
		return 1;
	}

	xsdk_log_write("blades startup 6: display %ux%u ready; starting audio", (unsigned)width, (unsigned)height);
	ui_audio_xsdk_start();
	xsdk_log_write("blades startup 7: audio start returned; entering frame loop");
	bool first_frame = true;
	bool wait_for_release = true;
	unsigned int status_frames = 0;
	struct timespec last_frame = { 0 };
	bool clock_ready = clock_gettime(CLOCK_MONOTONIC, &last_frame) == 0;
	uint64_t cpu_us=0,upload_us=0,wait_us=0,total_us=0,max_us=0;
	unsigned timing_frames=0;
	for (;;) {
		uint64_t frame_start=microseconds();
		BladesHeld held;
		uint32_t address;
		struct timespec now;
		float dt = 1.0f / 60.0f;
		if (clock_gettime(CLOCK_MONOTONIC, &now) == 0) {
			if (clock_ready) dt = (float)(now.tv_sec - last_frame.tv_sec) + (float)(now.tv_nsec - last_frame.tv_nsec) / 1000000000.0f;
			last_frame = now; clock_ready = true;
		}
		uint32_t buttons = read_pad(&held);
		bool system_ui = xsdk_system_ui_open() || (buttons & INPUT_GAMEPAD_GUIDE) != 0;
		blades_system_ui_state(system_ui);
		if (system_ui) wait_for_release = true;
		else if (wait_for_release && !buttons && !held.up && !held.down && !held.left && !held.right) wait_for_release = false;
		if (status_frames++ % 60 == 0 || held.guide) {
			if (first_frame) xsdk_log_write("blades startup 8: reading profile and clock");
			uint32_t user_index;
			char name[XSDK_XAM_GAMERTAG_BYTES] = { 0 };
			uint32_t user_result = xsdk_xam_signed_in_user(&user_index, name);
			if (user_result == ERROR_SUCCESS) blades_set_user(name);
			else if (user_result == ERROR_NO_SUCH_USER) blades_set_user(NULL);
			time_t wall_time = time(NULL);
			struct tm *local = localtime(&wall_time);
			if (local) blades_set_clock(local->tm_hour, local->tm_min);
			if (first_frame) xsdk_log_write("blades startup 9: profile and clock returned");
		}
		if (!wait_for_release) blades_frame(&held, dt);
		ui_audio_xsdk_play(blades_take_sound());
		/* Never terminate while XAM owns the display. Back's destination is unverified. */
		if (!system_ui && !wait_for_release && (buttons & INPUT_GAMEPAD_BACK))
			break;
		uint64_t cpu_end=microseconds();
		status = xsdk_present_begin(&address, BEGIN_TIMEOUT_MS);
		if (!NT_SUCCESS(status)) {
			xsdk_log_write("blades: xsdk_present_begin failed: 0x%08X", (unsigned)status);
			break;
		}
		uint64_t upload_start=microseconds();
		xsdk_texture_upload(&texture, 0, blades_pixels(), BLADES_WIDTH * 4);
		xsdk_gpu_invalidate(texture.address, texture.size_bytes);
		uint64_t upload_end=microseconds();
		xsdk_gpu_set_target(width, height);
		xsdk_gpu_set_depth_stencil(&depth_state);
		xsdk_gpu_set_shaders(&vertex, &pixel);
		xsdk_built_shader_set_literals(&screen_vert);
		xsdk_built_shader_set_literals(&screen_frag);
		xsdk_gpu_set_texture(SCREEN_FRAG_SAMPLER_0_0, &texture, &sampler_state);
		xsdk_gpu_set_vertices(0, xsdk_gpu_address(vertices), VERTICES_BYTES);
		xsdk_gpu_draw_indexed(XSDK_PRIMITIVE_TRIANGLE_LIST, xsdk_gpu_address(indices), INDICES, false);
		xsdk_gpu_resolve(address, width, height, 0xFF202020);
		xsdk_present_end();
		uint64_t frame_end=microseconds(),elapsed=frame_end-frame_start;
		cpu_us+=cpu_end-frame_start;wait_us+=upload_start-cpu_end;
		upload_us+=upload_end-upload_start;total_us+=elapsed;
		if(elapsed>max_us)max_us=elapsed;
		if(++timing_frames==300) {
			xsdk_log_write("blades timings us: frame avg=%u max=%u CPU=%u begin-wait=%u upload=%u (300 frames)",
				(unsigned)(total_us/300),(unsigned)max_us,(unsigned)(cpu_us/300),
				(unsigned)(wait_us/300),(unsigned)(upload_us/300));
			timing_frames=0;cpu_us=upload_us=wait_us=total_us=max_us=0;
		}
		if (first_frame) {
			xsdk_log_write("blades startup 10: first dashboard frame submitted");
			first_frame = false;
		}
	}

	ui_audio_xsdk_stop();
	xsdk_present_close();
	xsdk_texture_free(&texture);
	xsdk_gpu_free(indices);
	xsdk_gpu_free(vertices);
	xsdk_shader_free(&pixel);
	xsdk_shader_free(&vertex);
	xsdk_gpu_close();
	xsdk_log_write("blades: done");
	return 0;
}
