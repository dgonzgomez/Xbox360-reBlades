#include <stdbool.h>
#include <stdint.h>
#include "xsdk/gpu.h"
#include "xsdk/kernel.h"
#include "xsdk/log.h"
#include "xsdk/present.h"

/* A separate title so a failing dashboard does not hide the basic loader/display result.
 * Uses the same resolve/clear loop as the SDK hello sample. */
int main(void)
{
    uint32_t width = 0, height = 0;
    xsdk_log_write("boot-probe v1: entered main");
    xsdk_log_write("boot-probe: opening GPU");
    NTSTATUS status = xsdk_gpu_open();
    xsdk_log_write("boot-probe: GPU result 0x%08X", (unsigned)status);
    if (!NT_SUCCESS(status)) return 1;
    xsdk_log_write("boot-probe: opening 720p display");
    status = xsdk_present_open(XSDK_DISPLAY_720P, 2, 1, &width, &height);
    xsdk_log_write("boot-probe: display result 0x%08X, %ux%u", (unsigned)status,
        (unsigned)width, (unsigned)height);
    if (!NT_SUCCESS(status)) return 1;

    bool back_armed = false;
    bool first_frame = true;
    for (;;) {
        INPUT_STATE input = {0};
        if (XamInputGetState(0, 0, &input) != ERROR_SUCCESS)
            input = (INPUT_STATE){0};
        /* Ignore launch-time held buttons until they have all been released. */
        if (!input.gamepad.buttons) back_armed = true;
        if (back_armed && (input.gamepad.buttons & INPUT_GAMEPAD_BACK)) break;
        uint32_t address;
        status = xsdk_present_begin(&address, 5000);
        if (!NT_SUCCESS(status)) {
            xsdk_log_write("boot-probe: begin failed 0x%08X", (unsigned)status);
            break;
        }
        xsdk_gpu_resolve(address, width, height,
            (input.gamepad.buttons & INPUT_GAMEPAD_A) ? 0xFF207040 : 0xFF204080);
        xsdk_present_end();
        if (first_frame) {
            xsdk_log_write("boot-probe: first frame submitted; blue, hold A for green, Back exits");
            first_frame = false;
        }
    }
    xsdk_log_write("boot-probe: leaving loop, closing display");
    xsdk_present_close();
    (void)xsdk_gpu_close();
    return NT_SUCCESS(status) ? 0 : 1;
}
