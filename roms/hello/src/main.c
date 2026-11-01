// hello: the J64 starter ROM.
//
// Shows the build version, moves a box with the stick or D-pad, and logs to the
// debug channel when A is pressed. On a SummerCart64, `make deploy ROM=hello`
// streams those log lines back to your terminal over USB.
#include <libdragon.h>

#include "j64/util.h"

#define SCREEN_W 320
#define SCREEN_H 240
#define BOX      16

int main(void) {
    // Log over SC64 USB on hardware and to the emulator log (ISViewer) in ares.
    debug_init_usblog();
    debug_init_emulog();
    debugf("J64 hello %s booting\n", J64_VERSION);

    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, FILTERS_RESAMPLE);
    rdpq_init();
    joypad_init();
    rdpq_text_register_font(1, rdpq_font_load_builtin(FONT_BUILTIN_DEBUG_MONO));

    int x = (SCREEN_W - BOX) / 2;
    int y = (SCREEN_H - BOX) / 2;
    unsigned presses = 0;
    unsigned frame = 0;

    while (1) {
        joypad_poll();
        joypad_inputs_t in = joypad_get_inputs(JOYPAD_PORT_1);
        joypad_buttons_t pressed = joypad_get_buttons_pressed(JOYPAD_PORT_1);

        x += j64_deadzone(in.stick_x, 8) / 16 + in.btn.d_right - in.btn.d_left;
        y -= j64_deadzone(in.stick_y, 8) / 16 + in.btn.d_up - in.btn.d_down;
        x = j64_clampi(x, 0, SCREEN_W - BOX);
        y = j64_clampi(y, 0, SCREEN_H - BOX);

        if (pressed.a) {
            presses++;
            debugf("A pressed (%u) at %d,%d\n", presses, x, y);
        }

        surface_t *disp = display_get();
        rdpq_attach_clear(disp, NULL);

        rdpq_set_mode_fill(RGBA32(0xE0, 0x40, 0x40, 0xFF));
        rdpq_fill_rectangle(x, y, x + BOX, y + BOX);

        rdpq_set_mode_standard();
        rdpq_text_printf(NULL, 1, 24, 32, "Hello from J64");
        rdpq_text_printf(NULL, 1, 24, 48, "Version: %s", J64_VERSION);
        rdpq_text_printf(NULL, 1, 24, 64, "Frame %u   A presses %u", frame++, presses);
        rdpq_text_printf(NULL, 1, 24, 216, "Stick/D-pad: move   A: log");

        rdpq_detach_show();
    }
}
