// Copyright (C) 2026 Benjamin Froelich
// This file is part of https://github.com/bbeni/impedancer
// For conditions of distribution and use, see copyright notice in project root.
//
// Graphing Functions
//

#include "gra.h"
#include "string.h"
#include "stdio.h"
#include "assert.h"

Mui_Color _color_bg() {return mui_protos_theme_g.bg;}
Mui_Color _color_border() {return mui_protos_theme_g.border;}
Mui_Color _color_text() {return mui_protos_theme_g.text_muted;}

void _draw_grid(Mui_Rectangle plot_area, double x_min, double x_max, double y_min, double y_max, double x_step, double y_step, bool thick_y_zero) {
    // draw border
    mui_draw_rectangle_lines(plot_area, _color_border(), 2.0f);

    // draw grid
    float grid_x = mma_next_multiple_of(x_min, x_step);
    float grid_norm_x = (grid_x - x_min)/ (x_max - x_min);
    float x = plot_area.x + grid_norm_x * plot_area.width;
    float y1 = plot_area.y;
    float y2 = plot_area.y + plot_area.height;
    int iter = 0;
#define MAX_LINES 1000
    while (grid_x < x_max && iter < MAX_LINES) {
        mui_draw_line(x, y1, x, y2, 1.0f, _color_text());
        grid_x = mma_next_multiple_of(grid_x, x_step);
        grid_norm_x = (grid_x - x_min) / (x_max - x_min);
        x = plot_area.x + grid_norm_x * plot_area.width;
        iter++;
    }

    float grid_y = mma_next_multiple_of(y_min, y_step);
    float grid_norm_y = 1 - (grid_y - y_min)/ (y_max - y_min);
    float y = plot_area.y + grid_norm_y * plot_area.height;
    float x1 = plot_area.x;
    float x2 = plot_area.x + plot_area.width;
    iter = 0;
    while (grid_y < y_max && iter < MAX_LINES) {
        mui_draw_line(x1, y, x2, y, 1.0f, _color_text());
        grid_y = mma_next_multiple_of(grid_y, y_step);
        grid_norm_y = 1 - (grid_y - y_min) / (y_max - y_min);
        y = plot_area.y + grid_norm_y * plot_area.height;
        iter++;
    }
    if (thick_y_zero) {
        grid_norm_y = 1 - (0 - y_min) / (y_max - y_min);
        y = plot_area.y + grid_norm_y * plot_area.height;
        mui_draw_line(x1, y, x2, y, 3.0f, _color_text());
    }
}

// draw the grid and labels.
Mui_Rectangle gra_xy_plot_labels_and_grid(char* x_label, char* y_label, double x_min, double x_max, double y_min, double y_max, double x_step, double y_step, bool thick_y_zero, Mui_Rectangle place) {

    (void) y_label;

    float label_text_size = mui_protos_theme_g.label_text_size;

    // make space for label
    Mui_Rectangle x_label_place;
    Mui_Rectangle rest = mui_cut_bot(place, label_text_size, &x_label_place);
    Mui_Rectangle y_label_place;
    rest = mui_cut_left(rest, label_text_size, &y_label_place);

    size_t l = strlen(x_label);
    Mui_Vector2 m = mui_measure_text(mui_protos_theme_g.label_font, x_label, label_text_size, 0.1f, 0, l);
    Mui_Vector2 x_label_pos;
    x_label_pos.x = x_label_place.x + x_label_place.width/2 - m.x/2;
    x_label_pos.y = x_label_place.y;
    mui_draw_text_line(mui_protos_theme_g.label_font, x_label_pos, 0.1f, label_text_size, x_label, _color_text(), 0, l);

    Mui_Rectangle plot_area = rest;
    mui_draw_rectangle(plot_area, _color_bg());
    _draw_grid(plot_area, x_min, x_max, y_min, y_max, x_step, y_step, thick_y_zero);

    return plot_area;
}

void gra_xy_plot(double *x_data,
                 void *y_data,
                 double (* y_map)(size_t i, void *x),
                 size_t data_length,
                 char* x_label,
                 char* y_label,
                 double x_min, double x_max,
                 double y_min, double y_max,
                 double x_step, double y_step,
                 Mui_Color color,
                 Mui_Rectangle place)
{

    Mui_Rectangle plot_area = gra_xy_plot_labels_and_grid(x_label, y_label, x_min, x_max, y_min, y_max, x_step, y_step, true, place);

    for (size_t i = 0; i < data_length; i++) {
        double x = x_data[i];
        double y = y_map(i, y_data);
        if (x >= x_min && x <= x_max && y <= y_max && y >= y_min){
            float norm_x = (x - x_min)/ (x_max - x_min);
            float norm_y = (y - y_min)/ (y_max - y_min);
            Mui_Vector2 screen_coords;
            screen_coords.x = norm_x * plot_area.width + plot_area.x;
            screen_coords.y = (1 - norm_y) * plot_area.height + plot_area.y;
            mui_draw_circle(screen_coords, 2.0f, color);
        }
    }
}

void gra_xy_legend(char **labels, Mui_Color *colors, bool *mask, size_t n_labels_, Mui_Rectangle plot_area) {
    size_t n_labels = 0;
    for (size_t n = 0; n < n_labels_; n++) {
        if (mask[n]) n_labels++;
    }
    if (n_labels == 0) return;
    float legends_v_spacing = 0.1f;
    float legends_size = mui_protos_theme_g.label_text_size;
    float legends_padding = 10.0f;
    float legend_spacing = 10.0f;
    Mui_Rectangle  legends_rect;
    mui_cut_top(plot_area, n_labels * legends_size + (n_labels-1)*legends_v_spacing + 2*legends_padding + 2*legend_spacing, &legends_rect);
    legends_rect = mui_cut_left(legends_rect, legends_rect.width - 200, NULL);
    legends_rect = mui_shrink(legends_rect, legend_spacing);
    mui_draw_rectangle_rounded(legends_rect, 10.0f, _color_bg());
    mui_draw_rectangle_rounded_lines(legends_rect, 10.0f, _color_border(), 1.0f);
    legends_rect = mui_shrink(legends_rect, legends_padding);

    size_t index = 0;
    for (size_t i = 0; i < n_labels_; i++) {
        if (!mask[i]) continue;
        Mui_Vector2 pos;
        pos.x = legends_rect.x;
        pos.y = legends_rect.y + index * legends_size + index * legends_v_spacing;

        mui_draw_text_line(mui_protos_theme_g.label_font, pos, 0.1f, legends_size, labels[i], colors[i], 0, strlen(labels[i]));
        index++;
    }
}

// if y_map is NULL we assume y_data to be double
void gra_xy_plot_data_points(double *x_data, void *y_data, double (* y_map)(size_t i, void *x), size_t data_length,
                 double x_min, double x_max, double y_min, double y_max,
                 Mui_Color color, float pt_radius, Mui_Rectangle plot_area)
{
    for (size_t i = 0; i < data_length; i++) {
        double x = x_data[i];
        double y;
        if (y_map) {
            y = y_map(i, y_data);
        } else {
            y = ((double*)y_data)[i];
        }
        if (x >= x_min && x <= x_max && y <= y_max && y >= y_min){
            float norm_x = (x - x_min) / (x_max - x_min);
            float norm_y = (y - y_min) / (y_max - y_min);
            Mui_Vector2 screen_coords;
            screen_coords.x = norm_x * plot_area.width + plot_area.x;
            screen_coords.y = (1 - norm_y) * plot_area.height + plot_area.y;
            mui_draw_circle(screen_coords, pt_radius, color);
        }
    }
}

void gra_xy_plot_line(double x1, double y1, double x2, double y2,
                 double x_min, double x_max, double y_min, double y_max,
                 Mui_Color color, float line_thickness, Mui_Rectangle plot_area) {

    Mui_Vector2 screen_coords_1;
    Mui_Vector2 screen_coords_2;
    {
        float norm_x = (x1 - x_min) / (x_max - x_min);
        float norm_y = (y1 - y_min) / (y_max - y_min);
        screen_coords_1.x = norm_x * plot_area.width + plot_area.x;
        screen_coords_1.y = (1 - norm_y) * plot_area.height + plot_area.y;
    }

    {
        float norm_x = (x2 - x_min) / (x_max - x_min);
        float norm_y = (y2 - y_min) / (y_max - y_min);
        screen_coords_2.x = norm_x * plot_area.width + plot_area.x;
        screen_coords_2.y = (1 - norm_y) * plot_area.height + plot_area.y;
    }

    mui_draw_line(screen_coords_1.x, screen_coords_1.y, screen_coords_2.x, screen_coords_2.y, line_thickness, color);

}



void _gridded_draw_grid(Mui_Rectangle plot_area, float step_x, float step_y, float off_x, float off_y, float thickness) {
    float x_a = plot_area.x;
    float x_b = plot_area.x + plot_area.width;
    float x = x_a + off_x;
    for (int i = 0; i < 1000 && x <= x_b; i++) {
        mui_draw_line(x, plot_area.y, x, plot_area.y + plot_area.height, thickness, _color_text());
        x += step_x;
    }

    float y_a = plot_area.y;
    float y_b = plot_area.y + plot_area.height;
    float y = y_a + off_y;
    for (int i = 0; i < 1000 && y <= y_b; i++) {
        mui_draw_line(plot_area.x, y, plot_area.x + plot_area.width, y, thickness, _color_text());
        y += step_y;
    }
}

void _gridded_draw_small_ticks(Mui_Rectangle plot_area, float step_x, float step_y, float off_x, float off_y, float thickness, float length) {
    float x_a = plot_area.x;
    float x_b = plot_area.x + plot_area.width;
    float x = x_a + off_x;
    for (int i = 0; i < 1000 && x <= x_b; i++) {
        mui_draw_line(x, plot_area.y, x, plot_area.y + length, thickness, _color_text());
        mui_draw_line(x, plot_area.y + plot_area.height - length, x, plot_area.y + plot_area.height, thickness, _color_text());
        x += step_x;
    }

    float y_a = plot_area.y;
    float y_b = plot_area.y + plot_area.height;
    float y = y_a + off_y;
    for (int i = 0; i < 1000 && y <= y_b; i++) {
        mui_draw_line(plot_area.x, y, plot_area.x + length, y, thickness, _color_text());
        mui_draw_line(plot_area.x + plot_area.width - length, y, plot_area.x + plot_area.width, y, thickness, _color_text());
        y += step_y;
    }
}

void _gridded_draw_tick_labels(Mui_Rectangle plot_area,
    float step_x, float step_y, float off_x, float off_y,
    const char* fmt_x, const char* fmt_y, float x_left, float x_right, float y_top, float y_bot) {

    char buffer[24];

    float x_a = plot_area.x;
    float x_b = plot_area.x + plot_area.width;
    float x = x_a + off_x;
    int count_x = (x_b - x_a) / step_x + 1;
    for (int i = 0; i < count_x && x <= x_b; i++) {
        snprintf(buffer, 24, fmt_x, x_left + i * (x_right - x_left) / (count_x - 1));
        size_t l = mui_text_len(buffer, strlen(buffer));
        Mui_Vector2 text_measure = mui_measure_text(mui_protos_theme_g.font_small, buffer, mui_protos_theme_g.font_small_size, 0.0f, 0, l);
        Mui_Vector2 pos;
        pos.x = x - text_measure.x * 0.5f;
        pos.y = plot_area.y + plot_area.height + text_measure.y * 0.5f;
        mui_draw_text_line(mui_protos_theme_g.font_small, pos, 0.0f, mui_protos_theme_g.font_small_size, buffer, _color_text(), 0, l);
        x += step_x;
    }

    float y_a = plot_area.y;
    float y_b = plot_area.y + plot_area.height;
    float y = y_a + off_y;
    int count_y = (y_b - y_a) / step_y + 1;
    for (int i = 0; i < count_y && y <= y_b; i++) {
        snprintf(buffer, 24, fmt_y, y_top - i * (y_top - y_bot) / (count_y - 1));
        size_t l = mui_text_len(buffer, strlen(buffer));
        Mui_Vector2 text_measure = mui_measure_text(mui_protos_theme_g.font_small, buffer, mui_protos_theme_g.font_small_size, 0.0f, 0, l);
        Mui_Vector2 pos;
        pos.y = y - text_measure.y * 0.5f;
        pos.x = plot_area.x - text_measure.x - text_measure.y * 0.5f;
        mui_draw_text_line(mui_protos_theme_g.font_small, pos, 0.0f, mui_protos_theme_g.font_small_size, buffer, _color_text(), 0, l);
        y += step_y;
    }

}


Mui_Rectangle gra_gridded_xy_base(struct Gra_Gridded_Base_Arguments* args, Mui_Rectangle place) {

    float to_px = args->grid_unit_pixels;
    place.width = args->grid_w * to_px;
    place.height = args->grid_h * to_px;

    Mui_Rectangle y_axis_rect;
    Mui_Rectangle x_axis_rect;
    Mui_Rectangle rest = mui_cut_bot(place, args->grid_bot_axis_off * to_px, &x_axis_rect);
    rest = mui_cut_left(rest, args->grid_left_axis_off * to_px, &y_axis_rect);
    x_axis_rect = mui_cut_left(x_axis_rect, args->grid_left_axis_off * to_px, NULL);


    _gridded_draw_grid(rest, (args->grid_skip_x + 1) * to_px, (args->grid_skip_y + 1) * to_px, 0.0f, 0.0f, 1.0f);
    mui_draw_rectangle_lines(mui_shrink(rest, -1.0f), _color_text(), 2.0f);
    _gridded_draw_small_ticks(rest, to_px, to_px, 0.0f, 0.0f, 1.0f, to_px * 0.2f);
    _gridded_draw_tick_labels(
        rest, (args->grid_skip_x + 1) * to_px, (args->grid_skip_y + 1) * to_px,  0.0f, 0.0f,
        args->tick_x_label_fmt, args->tick_y_label_fmt, args->x_left, args->x_right, args->y_top, args->y_bot
    );

    struct Mui_Font* font = mui_protos_theme_g.font;
    float font_size = mui_protos_theme_g.font_size;
    {
        size_t l = mui_text_len(args->x_label, strlen(args->x_label));
        Mui_Vector2 measure = mui_measure_text(font, args->x_label, font_size, 0.0f, 0, l);
        // TODO: mui: refactor so that both functions start with the common arguments
        Mui_Vector2 pos = mui_center_of_rectangle(x_axis_rect);
        pos.x -= measure.x * 0.5f;
        pos.y -= measure.y * 0.5f;
        mui_draw_text_line(font, pos, 0.0f, font_size, args->x_label, _color_border(), 0, l);
    }

    {
        size_t l = mui_text_len(args->y_label, strlen(args->y_label));
        Mui_Vector2 measure = mui_measure_text(font, args->y_label, font_size, 0.0f, 0, l);
        // TODO: mui: refactor so that both functions start with the common arguments
        Mui_Vector2 pos = mui_center_of_rectangle(y_axis_rect);
        pos.x -= y_axis_rect.width * 0.5f - measure.y * 0.5f;
        mui_draw_text_line_angle(font, pos, 0.0f, font_size, args->y_label, _color_border(), 0, l, -90.0f);
    }

    return rest;
}

