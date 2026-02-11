// Copyright (C) 2026 Benjamin Froelich
// This file is part of https://github.com/bbeni/impedancer
// For conditions of distribution and use, see copyright notice in project root.
#include "mui.h"
#include "uti.h"
#include "math.h"
#include "stdbool.h"
#include "string.h"
#include "stdio.h"
#include "assert.h"
#include "stdlib.h"
#include "ctype.h"


// globals need to be updated by calling mui_update_input()
float _internal_global_time = 0.0f;
float _internal_global_previous_time = -0.001f;
Mui_Vector2 _internal_global_mouse_position = {0};

double mui_get_time() {
    return _internal_global_time;
}

double mui_previous_time() {
    return _internal_global_previous_time;
}

Mui_Vector2 mui_get_mouse_position() {
    return _internal_global_mouse_position;
}

void mui_update_core() {
    _internal_global_previous_time = _internal_global_time;
    _internal_global_time = mui_get_time_now();
    _internal_global_mouse_position = mui_get_mouse_position_now();
}


bool mui_load_resource_from_file(const char *file_path, size_t *out_size, void **data) {
#ifdef RESOURCE_PACKER
#include "resource_accessor.h"
    return ra_access_resource_by_file_path(file_path, (char**)data, out_size);
#else
    return uti_read_entire_file(file_path, (char**)data, out_size);
#endif
}


void mui_move_towards(float *x, float target, float speed, float dt) {
    if (*x == target) {
        return;
    }
    if (*x > target) {
        *x -= speed * dt;
        if (*x < target) {
            *x = target;
        }
    } else {
        *x += speed * dt;
        if (*x > target) {
            *x = target;
        }
    }
}

Mui_Rectangle mui_rectangle(float x, float y, float width, float height) {
    Mui_Rectangle r;
    r.x = x;
    r.y = y;
    r.width = width;
    r.height = height;
    return r;
}


// Shrink Rectangle by amount in all directions
Mui_Rectangle mui_shrink(Mui_Rectangle r, float amount) {
    r.width  -= 2*amount;
    r.height -= 2*amount;
    r.x      += amount;
    r.y      += amount;
    return r;
}


// Cut Rectangle from left side by amount - out_left can be NULL.
Mui_Rectangle mui_cut_left(Mui_Rectangle r, float amount, Mui_Rectangle *out_left) {
    if (out_left != NULL) {
        out_left->x = r.x;
        out_left->y = r.y;
        out_left->width = amount;
        out_left->height = r.height;
    }
    r.x += amount;
    r.width -= amount;
    return r;
}


// Cut Rectangle from right side by amount - out_right can be NULL.
Mui_Rectangle mui_cut_right(Mui_Rectangle r, float amount, Mui_Rectangle *out_right) {
    if (out_right != NULL) {
        out_right->x = r.x + r.width - amount;
        out_right->y = r.y;
        out_right->width = amount;
        out_right->height = r.height;
    }
    r.width -= amount;
    return r;
}


// Cut Rectangle from the top by amount - out_top can be NULL.
Mui_Rectangle mui_cut_top(Mui_Rectangle r, float amount, Mui_Rectangle *out_top) {
    if (out_top != NULL) {
        out_top->x = r.x;
        out_top->y = r.y;
        out_top->height = amount;
        out_top->width = r.width;
    }
    r.y += amount;
    r.height -= amount;
    return r;
}

// Cut Rectangle from the bot by amount - out_bot can be NULL.
Mui_Rectangle mui_cut_bot(Mui_Rectangle r, float amount, Mui_Rectangle *out_bot) {
    if (out_bot != NULL) {
        out_bot->x = r.x;
        out_bot->y = r.y + r.height - amount;
        out_bot->height = amount;
        out_bot->width = r.width;
    }
    r.height -= amount;
    return r;
}

Mui_Vector2 mui_center_of_rectangle(Mui_Rectangle rectangle) {
    Mui_Vector2 v;
    v.x = rectangle.x + 0.5f * rectangle.width;
    v.y = rectangle.y + 0.5f * rectangle.height;
    return v;
}

void mui_center_rectangle_inside_rectangle(Mui_Rectangle* inner, Mui_Rectangle outer) {
    inner->x = outer.x + (outer.width - inner->width) * 0.5f;
    inner->y = outer.y + (outer.height - inner->height)* 0.5f;
}


void mui_grid_22(Mui_Rectangle r, float factor_x, float factor_y, Mui_Rectangle *out_11, Mui_Rectangle *out_12, Mui_Rectangle *out_21, Mui_Rectangle *out_22) {
    if (out_11) {
        out_11->x = r.x;
        out_11->y = r.y;
        out_11->width = r.width * factor_x;
        out_11->height = r.height * factor_y;
    }
    if (out_12) {
        out_12->x = r.x;
        out_12->y = r.y + r.height * factor_y;
        out_12->width = r.width * factor_x;
        out_12->height = r.height * (1-factor_y);
    }
    if (out_21) {
        out_21->x = r.x + r.width * factor_x;
        out_21->y = r.y;
        out_21->width = r.width * (1-factor_x);
        out_21->height = r.height * factor_y;
    }
    if (out_22) {
        out_22->x = r.x + r.width * factor_x;
        out_22->y = r.y + r.height * factor_y;
        out_22->width = r.width * (1-factor_x);
        out_22->height = r.height * (1-factor_y);
    }
}

bool mui_is_inside_rectangle(Mui_Vector2 pos, Mui_Rectangle rect) {
    return pos.x >= rect.x &&
           pos.x <  rect.x + rect.width &&
           pos.y >= rect.y &&
           pos.y <  rect.y + rect.height;
}

size_t mui_text_len(const char* text, size_t size) {
    // TODO: implement unicode, for now only ascii
    (void)text;
    return size;
}

static bool window_grabbed = false;
static Mui_Vector2 window_grabbed_pos;
static Mui_Vector2 window_initial_os_position;
static Mui_Rectangle previous_window_rect;
static float window_x_hover_t = 0;
static float window_maximizer_hover_t = 0;
static float window_minimizer_hover_t = 0;
static float last_time = 0;
Mui_Rectangle mui_window_decoration(float height, bool movable, bool closeable, bool minimizable, bool maximizable, bool to_the_right, Mui_Rectangle window_rect) {

    float w_to_h_ratio = 3.71f;
    float w_component = w_to_h_ratio * height / 3;
    float running_sum_width = 0.0f;


    Mui_Color color = mui_protos_theme_g.text_muted;
    Mui_Color bg = mui_protos_theme_g.bg_light;
    Mui_Color color_hover = mui_protos_theme_g.primary;
    Mui_Color bg_hover = mui_protos_theme_g.primary_dark;
    float to_hover_speed = mui_protos_theme_g.animation_speed_to_hover;
    float to_normal_speed = mui_protos_theme_g.animation_speed_to_normal;

    // Update the time
    float dt = mui_get_time() - last_time;
    last_time = mui_get_time();

    float comp_x = 0;
    if (to_the_right)
        comp_x = window_rect.width - w_component;
    Mui_Rectangle comp_rect = mui_rectangle(window_rect.x + comp_x, window_rect.y, w_component, height);

    //
    // draw x
    //
    running_sum_width += comp_rect.width;
    if (closeable) {
        if (mui_is_inside_rectangle(mui_get_mouse_position(), comp_rect)) {
            if (mui_is_mouse_button_pressed(0) && closeable) {
                // TODO: kill app gracefully
                exit(0);
            }
            mui_move_towards(&(window_x_hover_t), 1, to_hover_speed, dt);
        } else {
            mui_move_towards(&(window_x_hover_t), 0, to_normal_speed, dt);
        }
    }

    Mui_Color b = mui_interpolate_color(bg, bg_hover, window_x_hover_t);
    Mui_Color f = mui_interpolate_color(color, color_hover, window_x_hover_t);

    mui_draw_rectangle(comp_rect, b);
    float x_s = height / 4;
    Mui_Vector2 center = mui_center_of_rectangle(comp_rect);
    mui_draw_line(center.x + x_s, center.y + x_s, center.x - x_s, center.y - x_s, 2.0f, f);
    mui_draw_line(center.x + x_s, center.y - x_s, center.x - x_s, center.y + x_s, 2.0f, f);

    //
    // draw maximizer
    //
    running_sum_width += comp_rect.width;
    if (to_the_right) {
        comp_rect.x -= w_component;
        comp_rect.x = ceilf(comp_rect.x);
    } else {
        comp_rect.x += w_component;
        comp_rect.x = floorf(comp_rect.x);
    }
    if (maximizable) {
        if (mui_is_inside_rectangle(mui_get_mouse_position(), comp_rect)) {
            if (mui_is_mouse_button_pressed(0)) {
                if (mui_is_window_maximized()){
                    mui_window_restore();
                } else {
                    mui_window_maximize();
                    previous_window_rect = window_rect;
                }
            }
            mui_move_towards(&(window_maximizer_hover_t), 1, to_hover_speed, dt);
        } else {
            mui_move_towards(&(window_maximizer_hover_t), 0, to_normal_speed, dt);
        }
    }

    b = mui_interpolate_color(bg, bg_hover, window_maximizer_hover_t);
    f = mui_interpolate_color(color, color_hover, window_maximizer_hover_t);

    mui_draw_rectangle(comp_rect, b);
    Mui_Rectangle m_rect = mui_shrink(comp_rect, height/4);
    mui_draw_rectangle_rounded_lines(m_rect, 5.0f, f, 2.0f);

    //
    // draw minimizer
    //
    if (to_the_right) {
        comp_rect.x -= w_component;
        comp_rect.x = ceilf(comp_rect.x);
    } else {
        comp_rect.x += w_component;
        comp_rect.x = floorf(comp_rect.x);
    }
    running_sum_width += comp_rect.width;


    if (minimizable) {
        if (mui_is_inside_rectangle(mui_get_mouse_position(), comp_rect)) {
            if (mui_is_mouse_button_pressed(0)) {
                    mui_window_minimize();
            }
            mui_move_towards(&(window_minimizer_hover_t), 1, to_hover_speed, dt);
        } else {
            mui_move_towards(&(window_minimizer_hover_t), 0, to_normal_speed, dt);
        }
    }

    b = mui_interpolate_color(bg, bg_hover, window_minimizer_hover_t);
    f = mui_interpolate_color(color, color_hover, window_minimizer_hover_t);

    mui_draw_rectangle(comp_rect, b);
    x_s = height / 4;
    center = mui_center_of_rectangle(comp_rect);
    mui_draw_line(center.x - x_s, center.y, center.x + x_s, center.y, 4.0f, f);

    //
    // draw dragging area
    //
    if (to_the_right) {
        comp_rect.x -= w_component * 3;
        comp_rect.x = ceilf(comp_rect.x);
    } else {
        comp_rect.x += w_component;
        comp_rect.x = floorf(comp_rect.x);
    }
    comp_rect.width = w_component * 3;
    running_sum_width += comp_rect.width;

    if (mui_is_inside_rectangle(mui_get_mouse_position(), comp_rect)) {
        if (mui_is_mouse_button_pressed(0) && movable) {
            window_grabbed = true;
            window_initial_os_position = mui_window_get_position();
            window_grabbed_pos = mui_get_mouse_position();
        }
    }

    if (mui_is_mouse_button_up(0)) {
        window_grabbed = false;
    }

    if (window_grabbed) {
        Mui_Vector2 current_mouse = mui_get_mouse_position();
        float dx = current_mouse.x - window_grabbed_pos.x;
        float dy = current_mouse.y - window_grabbed_pos.y;
        Mui_Vector2 current_win_pos = mui_window_get_position();

        if (dx != 0 || dy != 0) {
            mui_window_set_position(current_win_pos.x + dx, current_win_pos.y + dy);
        }
    }

    mui_draw_rectangle(comp_rect, bg);


    Mui_Rectangle ret_val;
    ret_val.x = to_the_right ? window_rect.x : window_rect.x + running_sum_width;
    ret_val.y = window_rect.y;
    ret_val.height = height;
    ret_val.width = window_rect.width - running_sum_width;

    return ret_val;
}

