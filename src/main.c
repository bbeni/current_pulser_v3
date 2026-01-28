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


int main() {

    int grid_w = 22;
    int grid_h = 21;
    int grid_pixel_unit = 50;

    int w, h;
    w = grid_w * grid_pixel_unit;
    h = grid_h * grid_pixel_unit;

    mui_open_window(w, h, 500, 200, "Current Pulser V3 Controller", 1.0f, MUI_WINDOW_RESIZEABLE | MUI_WINDOW_UNDECORATED, NULL);
    mui_init_themes(0, 0, false, "resources/font/NimbusSans-Regular.ttf");


    Mui_Button_State standby_button_state = {0};
    Mui_Button_State charge_button_state = {0};
    Mui_Button_State fire_button_state = {0};

    #define OFF 0
    #define CHARGE 1
    #define READY 2
    #define ERROR 3

    const char* STATUS_NAMES[4] = {"OFF", "CHARGE", "READY", "ERROR"};
    const Mui_Color STATUS_COLORS[4] = {MUI_GRAY, MUI_ORANGE, MUI_GREEN, MUI_RED};

    int ps_status = 0;
    int cb1_status = 0;
    int cb2_status = 0;

    while (!mui_window_should_close())
    {
        mui_update_core();

        w = mui_screen_width();
        h = mui_screen_height();

        mui_begin_drawing();
        mui_clear_background(mui_protos_theme_g.bg_light, NULL);

        Mui_Rectangle whole_screen = mui_rectangle(0, 0, w, h);

        const float decoration_height = grid_pixel_unit;
        Mui_Rectangle menu_bar_area = mui_window_decoration(decoration_height, true, true, true, true, true, whole_screen);
        Mui_Rectangle screen = mui_cut_top(whole_screen, decoration_height, NULL);
        mui_label(&mui_protos_theme_g, "PULSER V3 CONTROL", MUI_TEXT_ALIGN_DEFAULT, menu_bar_area);

        Mui_Rectangle work_area = mui_shrink(screen, grid_pixel_unit);

        // split space in top part, where the control interface goes. and bottom part, where the current pulse time series is displayed
        Mui_Rectangle top_area;
        Mui_Rectangle bottom_area = mui_cut_top(work_area, 5.5f * grid_pixel_unit, &top_area);

        // gap
        bottom_area = mui_cut_top(bottom_area, 0.5f * grid_pixel_unit, NULL);

        // current pulse time series
        Mui_Rectangle current_pulse_title_area;
        Mui_Rectangle time_series_area = mui_cut_top(bottom_area, grid_pixel_unit, &current_pulse_title_area);
        mui_label(&mui_protos_theme_g, "CURRENT PULSE", MUI_TEXT_ALIGN_LEFT, current_pulse_title_area);

        Mui_Rectangle graph_area = mui_cut_top(time_series_area, 1 * grid_pixel_unit, NULL);
        graph_area = mui_cut_right(graph_area, 4 * grid_pixel_unit, NULL);

        mui_draw_rectangle_lines(time_series_area, mui_protos_theme_g.text_muted, 2.0f);
        gra_xy_plot_labels_and_grid("t [ms]", "I [kA]", -0.2f, 1.2f, -5, 15, 0.2f, 5, true, graph_area);


        // control panels
        Mui_Rectangle panel_1_area;
        top_area = mui_cut_left(top_area, 6 * grid_pixel_unit, &panel_1_area);
        top_area = mui_cut_left(top_area, 1 * grid_pixel_unit, NULL);

        Mui_Rectangle panel_2_area;
        top_area = mui_cut_left(top_area, 6 * grid_pixel_unit, &panel_2_area);
        top_area = mui_cut_left(top_area, 1 * grid_pixel_unit, NULL);

        Mui_Rectangle panel_3_area;
        top_area = mui_cut_left(top_area, 6 * grid_pixel_unit, &panel_3_area);
        top_area = mui_cut_left(top_area, 1 * grid_pixel_unit, NULL);

        //
        // panel 1
        //
        {
            Mui_Rectangle standby_button_area;
            Mui_Rectangle power_supply_area = mui_cut_top(panel_1_area, 1 * grid_pixel_unit, &standby_button_area);

            if (mui_n_status_button(&standby_button_state, "STANDBY", STATUS_COLORS, 4, 0, standby_button_area)) {
                cb1_status = OFF;
            }



            power_supply_area = mui_cut_top(power_supply_area, 0.5f * grid_pixel_unit, NULL);
            Mui_Rectangle power_supply_title_area;
            power_supply_area = mui_cut_top(power_supply_area, 1 * grid_pixel_unit, &power_supply_title_area);

            mui_label(&mui_protos_theme_g, "POWER SUPPLY", MUI_TEXT_ALIGN_LEFT, power_supply_title_area);
            mui_draw_rectangle_lines(power_supply_area, mui_protos_theme_g.text_muted, 2.0f);

            // divide into 2:3:1
            Mui_Rectangle ui_label_rect;
            power_supply_area = mui_cut_left(power_supply_area, 2 * grid_pixel_unit, &ui_label_rect);
            Mui_Rectangle ui_rect;
            Mui_Rectangle ps_status_rect = mui_cut_left(power_supply_area, 3 * grid_pixel_unit, &ui_rect);

            ui_label_rect = mui_cut_top(ui_label_rect, 0.3333333333f * grid_pixel_unit, NULL);
            ui_rect = mui_cut_top(ui_rect, 0.3333333333f * grid_pixel_unit, NULL);

            Mui_Rectangle u_label_rect;
            ui_label_rect = mui_cut_top(ui_label_rect, 1 * grid_pixel_unit, &u_label_rect);
            ui_label_rect = mui_cut_top(ui_label_rect, 0.3333333333f * grid_pixel_unit, NULL);
            Mui_Rectangle i_label_rect;
            ui_label_rect = mui_cut_top(ui_label_rect, 1 * grid_pixel_unit, &i_label_rect);

            Mui_Rectangle u_rect;
            ui_rect = mui_cut_top(ui_rect, 1 * grid_pixel_unit, &u_rect);
            ui_rect = mui_cut_top(ui_rect, 0.3333333333f * grid_pixel_unit, NULL);
            Mui_Rectangle i_rect;
            ui_rect = mui_cut_top(ui_rect, 1 * grid_pixel_unit, &i_rect);

            mui_draw_rectangle_lines(u_rect, MUI_BLACK, 2.0f);
            mui_draw_rectangle_lines(i_rect, MUI_BLACK, 2.0f);
            mui_label(&mui_protos_theme_g, "1'654 V", MUI_TEXT_ALIGN_RIGHT, u_rect);
            mui_label(&mui_protos_theme_g, "295 mA", MUI_TEXT_ALIGN_RIGHT, i_rect);
            mui_label(&mui_protos_theme_g, "U", MUI_TEXT_ALIGN_CENTER, u_label_rect);
            mui_label(&mui_protos_theme_g, "I", MUI_TEXT_ALIGN_CENTER, i_label_rect);

            float ps_status_radius = grid_pixel_unit * 0.33333333f;
            Mui_Vector2 ps_status_center = mui_center_of_rectangle(ps_status_rect);
            mui_draw_circle(ps_status_center, ps_status_radius, MUI_GREEN);
            Mui_Rectangle ps_status_label_rect;
            mui_cut_bot(mui_cut_bot(ps_status_rect, 0.33333f * grid_pixel_unit, NULL), 1 * grid_pixel_unit, &ps_status_label_rect);
            mui_label(&mui_protos_theme_g, "ON", MUI_TEXT_ALIGN_CENTER, ps_status_label_rect);
        }

        //
        // panel 2
        //
        {
            Mui_Rectangle charge_button_area;
            Mui_Rectangle cap_bank_1_area = mui_cut_top(panel_2_area, 1 * grid_pixel_unit, &charge_button_area);

            if (mui_n_status_button(&charge_button_state, "CHARGE", STATUS_COLORS, 4, cb1_status, charge_button_area)) {
                cb1_status += 1;
                if (cb1_status > 2) cb1_status = READY;
            }


            Mui_Rectangle cb1_title_area;

            Mui_Rectangle cb1_labels_rect;
            Mui_Rectangle cb1_set_and_status_rect;
            Mui_Rectangle cb1_enable_rect;

            Mui_Rectangle cb1_status_rect;
            Mui_Rectangle cb1_status_label_rect;

            Mui_Rectangle label_set_rect;
            Mui_Rectangle label_status_rect;
            Mui_Rectangle cb1_set_rect;

            cap_bank_1_area = mui_cut_top(cap_bank_1_area, 0.5f * grid_pixel_unit, NULL);
            cap_bank_1_area = mui_cut_top(cap_bank_1_area, 1 * grid_pixel_unit, &cb1_title_area);

            mui_label(&mui_protos_theme_g, "CAP BANK 1", MUI_TEXT_ALIGN_LEFT, cb1_title_area);
            mui_draw_rectangle_lines(cap_bank_1_area, mui_protos_theme_g.text_muted, 2.0f);

            // divide into 2:3:1
            cap_bank_1_area = mui_cut_left(cap_bank_1_area, 2 * grid_pixel_unit, &cb1_labels_rect);
            cb1_enable_rect = mui_cut_left(cap_bank_1_area, 3 * grid_pixel_unit, &cb1_set_and_status_rect);

            cb1_labels_rect = mui_cut_top(cb1_labels_rect, 0.3333333333f * grid_pixel_unit, NULL);
            cb1_set_and_status_rect = mui_cut_top(cb1_set_and_status_rect, 0.3333333333f * grid_pixel_unit, NULL);
            cb1_labels_rect = mui_cut_top(cb1_labels_rect, 1 * grid_pixel_unit, &label_set_rect);
            cb1_labels_rect = mui_cut_top(cb1_labels_rect, 0.3333333333f * grid_pixel_unit, NULL);

            cb1_labels_rect = mui_cut_top(cb1_labels_rect, 1 * grid_pixel_unit, &label_status_rect);

            cb1_set_and_status_rect = mui_cut_top(cb1_set_and_status_rect, 1 * grid_pixel_unit, &cb1_set_rect);
            cb1_set_and_status_rect = mui_cut_top(cb1_set_and_status_rect, 0.3333333333f * grid_pixel_unit, NULL);
            cb1_set_and_status_rect = mui_cut_top(cb1_set_and_status_rect, 1 * grid_pixel_unit, &cb1_status_rect);

            mui_draw_rectangle_lines(cb1_set_rect, MUI_BLACK, 2.0f);
            mui_draw_rectangle_lines(cb1_status_rect, MUI_BLACK, 2.0f);
            mui_label(&mui_protos_theme_g, "1'654 V", MUI_TEXT_ALIGN_RIGHT, cb1_set_rect);
            mui_n_status_label(&mui_protos_theme_g, STATUS_NAMES[cb1_status], STATUS_COLORS, 4, cb1_status, MUI_TEXT_ALIGN_RIGHT, cb1_status_rect);
            mui_label(&mui_protos_theme_g, "SET", MUI_TEXT_ALIGN_CENTER, label_set_rect);
            mui_label(&mui_protos_theme_g, "STATUS", MUI_TEXT_ALIGN_CENTER, label_status_rect);

            float ps_status_radius = grid_pixel_unit * 0.33333333f;
            Mui_Vector2 ps_status_center = mui_center_of_rectangle(cb1_enable_rect);
            mui_draw_circle(ps_status_center, ps_status_radius, MUI_GREEN);

            mui_cut_bot(mui_cut_bot(cb1_enable_rect, 0.33333f * grid_pixel_unit, NULL), 1 * grid_pixel_unit, &cb1_status_label_rect);
            mui_label(&mui_protos_theme_g, "EN", MUI_TEXT_ALIGN_CENTER, cb1_status_label_rect);
        }

        //
        // panel 3
        //
        {


            Mui_Rectangle fire_button_area;
            Mui_Rectangle cap_bank_2_area = mui_cut_top(panel_3_area, 1 * grid_pixel_unit, &fire_button_area);


            // fire status
            int fire_status = OFF;
            if (cb1_status == READY) {
                fire_status = READY;
            }

            if (mui_n_status_button(&fire_button_state, "FIRE", STATUS_COLORS, 4, fire_status, fire_button_area)) {
                if (fire_status == READY) {
                    cb1_status = OFF;
                }
            }



            Mui_Rectangle cb2_title_area;

            Mui_Rectangle cb2_labels_rect;
            Mui_Rectangle cb2_set_and_status_rect;
            Mui_Rectangle cb2_enable_rect;

            Mui_Rectangle cb2_status_rect;
            Mui_Rectangle cb2_status_label_rect;

            Mui_Rectangle label_set_rect;
            Mui_Rectangle label_status_rect;
            Mui_Rectangle cb2_set_rect;

            cap_bank_2_area = mui_cut_top(cap_bank_2_area, 0.5f * grid_pixel_unit, NULL);
            cap_bank_2_area = mui_cut_top(cap_bank_2_area, 1 * grid_pixel_unit, &cb2_title_area);

            mui_label(&mui_protos_theme_g, "CAP BANK 2", MUI_TEXT_ALIGN_LEFT, cb2_title_area);
            mui_draw_rectangle_lines(cap_bank_2_area, mui_protos_theme_g.text_muted, 2.0f);

            // divide into 2:3:1
            cap_bank_2_area = mui_cut_left(cap_bank_2_area, 2 * grid_pixel_unit, &cb2_labels_rect);
            cb2_enable_rect = mui_cut_left(cap_bank_2_area, 3 * grid_pixel_unit, &cb2_set_and_status_rect);

            cb2_labels_rect = mui_cut_top(cb2_labels_rect, 0.3333333333f * grid_pixel_unit, NULL);
            cb2_set_and_status_rect = mui_cut_top(cb2_set_and_status_rect, 0.3333333333f * grid_pixel_unit, NULL);
            cb2_labels_rect = mui_cut_top(cb2_labels_rect, 1 * grid_pixel_unit, &label_set_rect);
            cb2_labels_rect = mui_cut_top(cb2_labels_rect, 0.3333333333f * grid_pixel_unit, NULL);

            cb2_labels_rect = mui_cut_top(cb2_labels_rect, 1 * grid_pixel_unit, &label_status_rect);

            cb2_set_and_status_rect = mui_cut_top(cb2_set_and_status_rect, 1 * grid_pixel_unit, &cb2_set_rect);
            cb2_set_and_status_rect = mui_cut_top(cb2_set_and_status_rect, 0.3333333333f * grid_pixel_unit, NULL);
            cb2_set_and_status_rect = mui_cut_top(cb2_set_and_status_rect, 1 * grid_pixel_unit, &cb2_status_rect);

            mui_draw_rectangle_lines(cb2_set_rect, MUI_BLACK, 2.0f);
            mui_draw_rectangle_lines(cb2_status_rect, MUI_BLACK, 2.0f);
            mui_label(&mui_protos_theme_g, "1'654 V", MUI_TEXT_ALIGN_RIGHT, cb2_set_rect);
            mui_n_status_label(&mui_protos_theme_g, STATUS_NAMES[cb2_status], STATUS_COLORS, 4, cb2_status, MUI_TEXT_ALIGN_RIGHT, cb2_status_rect);
            mui_label(&mui_protos_theme_g, "SET", MUI_TEXT_ALIGN_CENTER, label_set_rect);
            mui_label(&mui_protos_theme_g, "STATUS", MUI_TEXT_ALIGN_CENTER, label_status_rect);

            float ps_status_radius = grid_pixel_unit * 0.33333333f;
            Mui_Vector2 ps_status_center = mui_center_of_rectangle(cb2_enable_rect);
            mui_draw_circle(ps_status_center, ps_status_radius, STATUS_COLORS[OFF]);

            mui_cut_bot(mui_cut_bot(cb2_enable_rect, 0.33333f * grid_pixel_unit, NULL), 1 * grid_pixel_unit, &cb2_status_label_rect);
            mui_label(&mui_protos_theme_g, "EN", MUI_TEXT_ALIGN_CENTER, cb2_status_label_rect);

        }

        mui_end_drawing();
        uti_temp_reset();
    }

    mui_close_window();

    return 0;
}