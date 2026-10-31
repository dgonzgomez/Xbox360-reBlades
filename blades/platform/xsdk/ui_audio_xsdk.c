#include "ui_audio_xsdk.h"
#include "ui_audio.h"
#include "xsdk/audio.h"
#include "xsdk/kernel.h"
#include "xsdk/log.h"
#include "xsdk/thread.h"

static HANDLE feeder;
static int next_event;

static uint32_t feed(void *context)
{
    (void)context;
    const UiAudioClip *clip = 0;
    unsigned int position = 0;
    for (;;) {
        float *frame;
        NTSTATUS status = xsdk_audio_next_frame(100, &frame);
        if (status == STATUS_IO_TIMEOUT) continue;
        if (!NT_SUCCESS(status)) return 0;
        int event = __atomic_exchange_n(&next_event, 0, __ATOMIC_RELAXED);
        if (event) {
            int index = event == UI_SOUND_BLADE ? 1 : event == UI_SOUND_SELECT ? 2 : event == UI_SOUND_GUIDE_OPEN ? 3 : event == UI_SOUND_GUIDE_CLOSE ? 4 : 0;
            clip = &ui_audio_clips[index]; position = 0;
        }
        for (unsigned int sample = 0; sample < XSDK_AUDIO_FRAME_SAMPLES; sample++) {
            float value = clip && position < clip->count ? clip->samples[position++] * (0.75f / 32768.0f) : 0;
            for (unsigned int channel = 0; channel < XSDK_AUDIO_CHANNELS; channel++)
                frame[channel * XSDK_AUDIO_FRAME_SAMPLES + sample] = channel <= XSDK_AUDIO_FRONT_RIGHT ? value : 0;
        }
        xsdk_audio_queue_frame();
    }
}

void ui_audio_xsdk_start(void)
{
    NTSTATUS status = xsdk_audio_open(0);
    if (!NT_SUCCESS(status)) { xsdk_log_write("blades: audio open failed: 0x%08X", (unsigned)status); return; }
    status = xsdk_thread_create(&feeder, 64 * 1024, feed, 0);
    if (!NT_SUCCESS(status)) {
        feeder = 0; xsdk_audio_close();
        xsdk_log_write("blades: audio feeder failed: 0x%08X", (unsigned)status);
    }
}
void ui_audio_xsdk_play(int event)
{
    if (feeder && event) __atomic_store_n(&next_event, event, __ATOMIC_RELAXED);
}
void ui_audio_xsdk_stop(void)
{
    if (!feeder) return;
    xsdk_audio_close();
    const int64_t timeout = -50000000;
    NTSTATUS status = NtWaitForSingleObjectEx(feeder, USER_MODE, 0, &timeout);
    if (status != STATUS_SUCCESS) xsdk_log_write("blades: audio feeder join: 0x%08X", (unsigned)status);
    NtClose(feeder); feeder = 0;
}
