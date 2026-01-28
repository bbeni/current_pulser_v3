// Copyright (C) 2026 Benjamin Froelich
// This file is part of https://github.com/bbeni/impedancer
// For conditions of distribution and use, see copyright notice in project root.

#include "stdio.h"
#include "math.h"
#include "stdbool.h"
#include "stdlib.h"
#include "string.h"
#include "assert.h"

#include "mui.h"
#include "gra.h"
#include "uti.h"


void draw_coil(Mui_Rectangle area, size_t n_windings) {
    mui_draw_rectangle(area, MUI_GRAY);
    float y = area.height * 0.1f;
    mui_draw_line(area.x + area.width * 0.9f, area.y + y, area.x + area.width, area.y + y, 4.0f, MUI_BLACK);
    y = area.height * 0.9f;
    mui_draw_line(area.x + area.width * 0.9f, area.y + y, area.x + area.width, area.y + y, 4.0f, MUI_BLACK);

}

void draw_cable_horizontal_first(float x1, float y1, float x2, float y2) {
    mui_draw_line(x1, y1, x2, y1, 4.0f, MUI_BLACK);
    mui_draw_line(x2, y1, x2, y2, 4.0f, MUI_BLACK);
}

int main() {

    int w, h;
    w = 1900;
    h = 1100;

    mui_open_window(w, h, 10, 10, "Current Pulser V3 Controller", 1.0f, MUI_WINDOW_RESIZEABLE | MUI_WINDOW_UNDECORATED, NULL);
    mui_init_themes(0, 0, false, "resources/font/NimbusSans-Regular.ttf");


    Mui_Button_State explode_button = {0};
    bool open = false;

    while (!mui_window_should_close())
    {
        mui_update_core();

        w = mui_screen_width();
        h = mui_screen_height();

        mui_begin_drawing();
        mui_clear_background(mui_protos_theme_g.bg_dark, NULL);


        Mui_Rectangle whole_screen = mui_rectangle(0, 0, w, h);
        const float decoration_height = 36.0f;
        Mui_Rectangle menu_bar_area = mui_window_decoration(decoration_height, true, true, true, true, true, whole_screen);
        Mui_Rectangle screen = mui_cut_top(whole_screen, decoration_height, NULL);
        mui_draw_rectangle(menu_bar_area, MUI_RED);
        mui_label(&mui_protos_theme_g, "Current Pulser V3 Controller", menu_bar_area);


        Mui_Rectangle button_area = mui_shrink(screen, 500);
        if(mui_button(&explode_button, "Explode", button_area)) {
            open = !open;
        }

        if (open) {

            Mui_Rectangle coil1;
            coil1.width = screen.width * 0.1f;
            coil1.height = screen.width * 0.05f;
            coil1.x = screen.x + screen.width * 0.6f;
            coil1.y = screen.y + (screen.height - coil1.height) * 0.5f;
            coil1.height = screen.width * 0.05f;
            draw_coil(coil1, 10);
            float cable_x1 = coil1.x + coil1.width;
            float cable_x2 = coil1.x + coil1.width + 100;
            float cable_y1 = coil1.y + coil1.height * 0.1f;
            float cable_y2 = coil1.y + coil1.height * 0.1f - 80;
            draw_cable_horizontal_first(cable_x1, cable_y1, cable_x2, cable_y2);

            cable_x2 = coil1.x + coil1.width + 100 + coil1.height * 0.8f;
            cable_y1 = coil1.y + coil1.height * 0.9f;
            draw_cable_horizontal_first(cable_x1, cable_y1, cable_x2, cable_y2);
        }

        mui_end_drawing();
        uti_temp_reset();
    }

    mui_close_window();

    return 0;
}