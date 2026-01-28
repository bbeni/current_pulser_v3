// Copyright (C) 2026 Benjamin Froelich
// This file is part of https://github.com/bbeni/impedancer
// For conditions of distribution and use, see copyright notice in project root.
//
// Graphing Functions
//

#include "gra.h"
#include "string.h"

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
