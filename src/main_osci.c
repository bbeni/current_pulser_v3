#include "mui.h"
#include "uti.h"

#include "osci_control.h"


int main() {

    int grid_w = 22;
    int grid_h = 15;
    int grid_pixel_unit = 50;

    int w, h;
    w = grid_w * grid_pixel_unit;
    h = grid_h * grid_pixel_unit;

    mui_open_window(w, h, 500, 200, "PULSER V3 OSCILLOSCOPE", 1.0f, MUI_WINDOW_RESIZEABLE /* | MUI_WINDOW_UNDECORATED */, NULL);
    mui_init_themes(0, 0, false, "resources/font/NimbusSans-Regular.ttf");

    struct Oscilloscope_Settings settings;
    settings.sample_rate  = 7692300.0f;  // Hz
    settings.v_pk_to_pk = 5.0f;          // volts
    settings.trigger_mode = false;
    settings.trigger_channel = 0;
    settings.trigger_level = 0.05f;      // volts
    settings.trigger_position = 0.0008;  // in seconds
    settings.trigger_auto_timeout = 1e23;     // in seconds
    settings.trigger_type = OSC_TRIGGER_TYPE_EDGE;
    settings.trigger_condition = OSC_TRIGGER_CONDITION_RISING_POSITIVE;
    struct Oscilloscope_State oscilloscope_state = {0};
    oscilloscope_setup(&oscilloscope_state, &settings);

    struct Oscilloscope_Ui_Settings ui_settings;
    struct Oscilloscope_Ui oscilloscope_ui_state = {0};
    oscilloscope_ui_setup(&oscilloscope_ui_state, &ui_settings, grid_pixel_unit);

    while (!mui_window_should_close())
    {
        mui_update_core();

        oscilloscope_ui_update(&oscilloscope_ui_state, &oscilloscope_state, "./osci_data");

        w = mui_screen_width();
        h = mui_screen_height();

        mui_begin_drawing();
        mui_clear_background(mui_protos_theme_g.bg_light, NULL);

        Mui_Rectangle whole_screen = mui_rectangle(0, 0, w, h);

        const float decoration_height = grid_pixel_unit;
        //Mui_Rectangle menu_bar_area = mui_window_decoration(decoration_height, true, true, true, true, true, whole_screen);
        Mui_Rectangle screen = mui_cut_top(whole_screen, decoration_height, NULL);
        //mui_label(&mui_protos_theme_g, "PULSER V3 OSCILLOSCOPE", MUI_TEXT_ALIGN_DEFAULT, menu_bar_area);

        Mui_Rectangle scope_rect = mui_shrink(screen, grid_pixel_unit);

        oscilloscope_ui_draw(scope_rect, grid_pixel_unit, &oscilloscope_ui_state, &oscilloscope_state, &settings);


        mui_end_drawing();
        uti_temp_reset();
    }

    oscilloscope_destroy(&oscilloscope_state);
    mui_close_window();

    return 0;
}