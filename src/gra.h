// Copyright (C) 2026 Benjamin Froelich
// This file is part of https://github.com/bbeni/impedancer
// For conditions of distribution and use, see copyright notice in project root.
//
// Graphing Functions
//
#ifndef GRA_H_
#define GRA_H_

#include "mui.h"
#include "mma.h"


//
// Basic XY plot
//

// draw a complete xy plot
void gra_xy_plot(double *x_data, void *y_data, double (* y_map)(size_t i, void *x), size_t data_length,
    char* x_label, char* y_label, double x_min, double x_max, double y_min, double y_max,
    double x_step, double y_step, Mui_Color color, Mui_Rectangle place);
// draw the grid and labels first. Returns plot_area rectangle.
Mui_Rectangle gra_xy_plot_labels_and_grid( char* x_label, char* y_label, double x_min, double x_max,
    double y_min, double y_max, double x_step, double y_step, bool thick_y_zero, Mui_Rectangle place);
// draw all xy data. if y_map is NULL we assume y_data to be double
void gra_xy_plot_data_points(double *x_data, void *y_data, double (* y_map)(size_t i, void *x), size_t data_length,
                 double x_min, double x_max, double y_min, double y_max,
                 Mui_Color color, float pt_radius, Mui_Rectangle plot_area);
// draw a line
void gra_xy_plot_line(double x1, double y1, double x2, double y2,
                 double x_min, double x_max, double y_min, double y_max,
                 Mui_Color color, float line_thickness, Mui_Rectangle plot_area);
// draw the legend last
void gra_xy_legend(char **labels, Mui_Color *colors, bool *mask, size_t n_labels_, Mui_Rectangle plot_area);

//
// Smith chart plot
//

// draw base of the smith chart
void draw_smith_grid(bool plot_reactance_circles, bool plot_admittance_circles, double *custom_cicles,
    size_t n_custom_circles, Mui_Rectangle plot_area);
// fmt_marker options: 'o': circles, '-': lines
void gra_smith_plot_data(double *f_data, struct Complex *z_data, size_t data_length,
                 double f_min, double f_max, Mui_Color color,
                 char fmt_marker, double marker_size, Mui_Rectangle plot_area);

//
// Gridded XY
//

struct Gra_Gridded_Base_Arguments {
    float grid_unit_pixels;
    size_t grid_w;
    size_t grid_h;
    size_t grid_left_axis_off;
    size_t grid_bot_axis_off;
    size_t grid_skip_x;
    size_t grid_skip_y;
    double x_left;
    double x_right;
    double y_bot;
    double y_top;
    char* x_label;
    char* y_label;
    bool thick_y_zero;
    const char* tick_x_label_fmt;
    const char* tick_y_label_fmt;
};

Mui_Rectangle gra_gridded_xy_base(struct Gra_Gridded_Base_Arguments* args, Mui_Rectangle place);





#endif //GRA_H_
