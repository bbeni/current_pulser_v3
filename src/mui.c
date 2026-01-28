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


// globals need to be updated by calling mui_update_input()
float mui_global_time_internal = 0.0f;
Mui_Vector2 mui_global_mouse_position_internal = {0};

double mui_get_time() {return mui_global_time_internal;}
Mui_Vector2 mui_get_mouse_position() {return mui_global_mouse_position_internal;}
void mui_update_core() {
    mui_global_mouse_position_internal = mui_get_mouse_position_now();
    mui_global_time_internal = mui_get_time_now();
}


bool mui_load_resource_from_file(const char *file_path, size_t *out_size, void **data) {
#ifdef RESOURCE_PACKER
#include "resource_accessor.h"
    return ra_access_resource_by_file_path(file_path, (char**)data, out_size);
#else
    return uti_read_entire_file(file_path, (char**)data, out_size);
#endif
}


#define MAX_FONTS_LOADED 128
struct Mui_Font* mui_font_catalog_g[MAX_FONTS_LOADED];
size_t mui_font_catalog_length_g = 0;
bool mui_load_ttf_font_for_theme(const char *font_file, Mui_Theme* theme) {

    size_t size;
    void *data;
    bool success = mui_load_resource_from_file(font_file, &size, &data);
    if (!success) {
        printf("ERROR: loading resource (%s) so we are not loading fonts..\n", font_file);
        return false;
    }

    if (mui_font_catalog_length_g + 3 >= MAX_FONTS_LOADED) {
        printf("ERROR: mui_font_catalog_length_g would exceed MAX_FONTS_LOADED (%zu/%u).. font not loaded !!\n", mui_font_catalog_length_g, MAX_FONTS_LOADED);
        return false;
    }

    mui_font_catalog_g[mui_font_catalog_length_g    ] = mui_load_font_ttf(data, size, theme->font_size);
    mui_font_catalog_g[mui_font_catalog_length_g + 1] = mui_load_font_ttf(data, size, theme->label_text_size);
    mui_font_catalog_g[mui_font_catalog_length_g + 2] = mui_load_font_ttf(data, size, theme->textinput_text_size);
    theme->font = mui_font_catalog_g[mui_font_catalog_length_g];
    theme->label_font = mui_font_catalog_g[mui_font_catalog_length_g + 1];
    theme->textinput_font = mui_font_catalog_g[mui_font_catalog_length_g + 2];
    mui_font_catalog_length_g += 3;

    return true;
}

void mui_load_latest_fonts_for_theme(Mui_Theme *theme) {
    assert(mui_font_catalog_length_g >= 3);
    theme->font = mui_font_catalog_g[mui_font_catalog_length_g - 3];
    theme->label_font = mui_font_catalog_g[mui_font_catalog_length_g - 2];
    theme->textinput_font = mui_font_catalog_g[mui_font_catalog_length_g - 1];
}

//
// state intialization
//

Mui_Button_State mui_button_state() {
    Mui_Button_State state;
    state.hover_t = 0.0f;
    state.last_time = 0.0f;
    state.theme = &mui_protos_theme_g;
    return state;
}

Mui_Checkbox_State mui_checkbox_state() {
    Mui_Checkbox_State state;
    state.hover_t = 0.0f;
    state.last_time = 0.0f;
    state.checked = false;
    state.theme = &mui_protos_theme_g;
    return state;
}

Mui_Slider_State mui_slider_state() {
    Mui_Slider_State state;
    state.hover_t = 0.0f;
    state.last_time = 0.0f;
    state.value = 0.0f;
    state.grabbed = false;
    state.theme = &mui_protos_theme_g;
    return state;
}

Mui_Collapsable_Section_State mui_collapsable_state() {
    Mui_Collapsable_Section_State state;
    state.hover_t = 0.0f;
    state.last_time = 0.0f;
    state.open = false;
    state.theme = &mui_protos_theme_g;
    return state;
}

Mui_Textinput_State mui_textinput_state() {
    Mui_Textinput_State state;
    for (size_t i = 0; i < MUI_TEXTINPUT_CAP; i++) {
        state.buf[MUI_TEXTINPUT_CAP + 1] = '\0';
    }
    state.count = 0;
    state.cursor = 0;
    state.active = false;
    state.theme = &mui_protos_theme_g;
    return state;
}


/* OKLAB maps

from RGB linear to lms
M1 =
{{0.4122214708, 0.5363325363, 0.0514459929},
 {0.2119034982, 0.6806995451, 0.1073969566},
 {0.0883024619, 0.2817188376, 0.6299787005}}
M1' =
{{4.07674, -3.30771, 0.23097}, {-1.26844, 2.60976, -0.341319}, {-0.00419609, -0.703419, 1.70761}}

lms^(1/3)
lms^(3) '

M2 = ...
M2' = ...
*/
float linear_to_srgb(float x) {
    if (x <= 0.0031308f) return 12.92f * x;
    return 1.055f * powf(x, 1.0f / 2.4f) - 0.055f;
}

Mui_Color mui_oklch_to_rgb(float l, float c, float h) {
    float a = c * cosf(h * 2 * M_PI / 360);
    float b = c * sinf(h * 2 *M_PI / 360);

    // M2' [L a b]
    float lp = l + a * +0.3963377774 + b * +0.2158037573;
    float mp = l + a * -0.1055613458 + b * -0.0638541728;
    float sp = l + a * -0.0894841775 + b * -1.2914855480;

    // l'm's'^(3)
    lp = lp*lp*lp;
    mp = mp*mp*mp;
    sp = sp*sp*sp;

    // M1' [l' m' s']
    float r_lin = lp * +4.0767416621 + mp * -3.3077115913 + sp * +0.2309699292;
    float g_lin = lp * -1.2684380046 + mp * +2.6097574011 + sp * -0.3413193965;
    float b_lin = lp * -0.0041960863 + mp * -0.7034186147 + sp * +1.7076147010;

    return (Mui_Color) {
        .r = (uint8_t)roundf(fmaxf(0, fminf(1, linear_to_srgb(r_lin))) * 255),
        .g = (uint8_t)roundf(fmaxf(0, fminf(1, linear_to_srgb(g_lin))) * 255),
        .b = (uint8_t)roundf(fmaxf(0, fminf(1, linear_to_srgb(b_lin))) * 255),
        .a = 255
    };
}


float mui_hue_to_rgb(float p, float q, float t) {
    if (t < 0.0f) t += 1;
    if (t > 1.0f) t -= 1;
    if (t < 1.0f/6) return p + (q - p) * 6 * t;
    if (t < 1.0f/2) return q;
    if (t < 2.0f/3) return p + (q - p) * (2.0f/3 - t) * 6;
    return p;
}

static inline Mui_Color mui_hsl_to_rgb(float h, float s, float l) {
    float r, g, b;
    if (s == 0) r = g = b = l;
    else {
        float q = l < 0.5f ? l * (1 + s) : l + s - l * s;
        float p = 2 * l - q;
        r = mui_hue_to_rgb(p, q, h + 1.0f/3);
        g = mui_hue_to_rgb(p, q, h);
        b = mui_hue_to_rgb(p, q, h - 1.0f/3);
    }
    return (Mui_Color) {
        .r = round(r * 255),
        .g = round(g * 255),
        .b = round(b * 255),
        .a = 255
    };
}

#define _OKLCH(l, c, h) mui_oklch_to_rgb((l), (c), (h))

Mui_Theme mui_protos_theme_dark_generate(float bg_hue, float bg_chroma) {
    return (Mui_Theme) {

        .bg_dark      = _OKLCH(0.25f, bg_chroma, 243 + bg_hue),
        .bg           = _OKLCH(0.32f, bg_chroma, 243 + bg_hue),
        .bg_light     = _OKLCH(0.4f, bg_chroma, 243 + bg_hue),
        .text         = _OKLCH(0.95f, bg_chroma, 243 + bg_hue),
        .text_muted   = _OKLCH(0.5f, bg_chroma, 243 + bg_hue),
        .border       = _OKLCH(0.45f, bg_chroma, 243 + bg_hue),
        .primary      = _OKLCH(0.7f, bg_chroma + 0.1, 243 + bg_hue),
        .primary_dark = _OKLCH(0.6f, bg_chroma + 0.1, 243 + bg_hue),

        .border_thickness = 2.0f,
        .font_size = 32.0f,
        .label_text_size = 32.0f,
        .textinput_text_size = 32.0f,

        .slider_thickness = 8.0f,
        .slider_wagon_width = 32.0f,
        .slider_wagon_height = 32.0f,
        .slider_wagon_corner_radius = 6.0f,
        .slider_wagon_border_thickness = 2.0f,

        .animation_speed_to_hover = 18.0f,
        .animation_speed_to_normal = 11.0f,
        .corner_radius = 4.0f,
    };
}

Mui_Theme mui_protos_theme_light_generate(float bg_hue, float bg_chroma) {
    return (Mui_Theme) {

        .bg_dark      = _OKLCH(0.75f, bg_chroma, 243 + bg_hue),
        .bg           = _OKLCH(0.85f, bg_chroma, 243 + bg_hue),
        .bg_light     = _OKLCH(0.95f, bg_chroma, 243 + bg_hue),
        .text         = _OKLCH(0.05f, bg_chroma, 243 + bg_hue),
        .text_muted   = _OKLCH(0.5f, bg_chroma, 243 + bg_hue),
        .border       = _OKLCH(0.55f, bg_chroma, 243 + bg_hue),
        .primary      = _OKLCH(0.7f, bg_chroma+0.09, 243 + bg_hue),
        .primary_dark = _OKLCH(0.6f, bg_chroma+0.09, 243 + bg_hue),

        .border_thickness = 2.0f,
        .font_size = 32.0f,
        .label_text_size = 32.0f,
        .textinput_text_size = 32.0f,

        .slider_thickness = 8.0f,
        .slider_wagon_width = 32.0f,
        .slider_wagon_height = 32.0f,
        .slider_wagon_corner_radius = 6.0f,
        .slider_wagon_border_thickness = 2.0f,

        .animation_speed_to_hover = 18.0f,
        .animation_speed_to_normal = 11.0f,
        .corner_radius = 4.0f,
    };
}

#undef _OKLCH

Mui_Theme mui_protos_theme_g;
Mui_Theme mui_protos_theme_dark_g;
Mui_Theme mui_protos_theme_light_g;

// if ttf_file_name is NULL take latest fonts.
void mui_init_themes(float chroma_bg, float bg_hue, bool dark, char* ttf_file_name) {
    mui_protos_theme_dark_g = mui_protos_theme_dark_generate(bg_hue, chroma_bg);
    mui_protos_theme_light_g = mui_protos_theme_light_generate(bg_hue, chroma_bg);

    if (ttf_file_name) {
        mui_load_ttf_font_for_theme(ttf_file_name, &mui_protos_theme_dark_g);
        mui_load_ttf_font_for_theme(ttf_file_name, &mui_protos_theme_light_g);
    } else {
        mui_load_latest_fonts_for_theme(&mui_protos_theme_dark_g);
        mui_load_latest_fonts_for_theme(&mui_protos_theme_light_g);
    }
    if (dark) mui_protos_theme_g = mui_protos_theme_dark_g;
    else mui_protos_theme_g = mui_protos_theme_light_g;
}



Mui_Color mui_interpolate_color(Mui_Color a, Mui_Color b, float t) {

    a.r = (1.0f - t) * a.r + t * b.r;
    a.g = (1.0f - t) * a.g + t * b.g;
    a.b = (1.0f - t) * a.b + t * b.b;
    a.a = (1.0f - t) * a.a + t * b.a;

    return a;
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

void mui_label(Mui_Theme *theme, char *text, Mui_Rectangle place) {
    if (theme == NULL) {
        theme = &mui_protos_theme_g;
    }

    mui_draw_rectangle_rounded(place, theme->corner_radius, theme->bg);

    Mui_Vector2 position;
    position.x = place.x + 20;
    position.y = place.y + place.height / 2 - theme->label_text_size / 2;

    int l = mui_text_len(text, strlen(text));
    mui_draw_text_line(theme->label_font, position, 0.1, theme->label_text_size, text, theme->text, 0, l);
}

bool mui_collapsable_section(Mui_Collapsable_Section_State *state, char* text, Mui_Rectangle place) {
    Mui_Theme *theme = state->theme;
    if (theme == NULL) theme = &mui_protos_theme_g;

    // Update the time
    float dt = mui_get_time() - state->last_time;
    state->last_time = mui_get_time();

    if (mui_is_inside_rectangle(mui_get_mouse_position(), place)) {
        if (mui_is_mouse_button_pressed(0)) {
            state->open = !state->open;
        }
        mui_move_towards(&(state->hover_t), 1, theme->animation_speed_to_hover, dt);
    } else {
        mui_move_towards(&(state->hover_t), 0, theme->animation_speed_to_normal, dt);
    }

    Mui_Color bg = mui_interpolate_color(theme->bg, theme->bg_light, state->hover_t);
    Mui_Color bc = mui_interpolate_color(theme->bg, theme->primary, state->hover_t);
    Mui_Color fg = mui_interpolate_color(theme->text_muted, theme->text, state->hover_t);

    Mui_Rectangle triangle_space;
    Mui_Rectangle text_space = mui_cut_left(place, place.height, &triangle_space); // place for triangle

    float border_thickness = 2.0f;
    mui_draw_rectangle_rounded(mui_shrink(place, border_thickness), theme->corner_radius, bg);
    mui_draw_rectangle_rounded_lines(mui_shrink(place, border_thickness), theme->corner_radius, bc, border_thickness);
    float x_s = triangle_space.height / 4;
    Mui_Vector2 center = mui_center_of_rectangle(triangle_space);

    if (state->open) {
        //
        mui_draw_line(center.x, center.y + x_s, center.x - x_s, center.y - x_s * sqrtf(0.5f), 2.0f, fg);
        mui_draw_line(center.x, center.y + x_s, center.x + x_s, center.y - x_s * sqrtf(0.5f), 2.0f, fg);
    } else {
        mui_draw_line(center.x + x_s, center.y, center.x - x_s * sqrtf(0.5f), center.y + x_s, 2.0f, fg);
        mui_draw_line(center.x + x_s, center.y, center.x - x_s * sqrtf(0.5f), center.y - x_s, 2.0f, fg);
    }

    if (text) {
        float font_size = theme->font_size;
        Mui_Vector2 pos;
        pos.x = text_space.x;
        pos.y = text_space.y + (text_space.height - font_size) * 0.5f;
        mui_draw_text_line(theme->font, pos, 0.1f, theme->font_size, text, fg, 0, strlen(text));
    }

    return state->open;
}

void mui_checkbox(Mui_Checkbox_State *state, const char *text, Mui_Rectangle place) {

    Mui_Theme *theme = state->theme;
    if (theme == NULL) theme = &mui_protos_theme_g;

    // Update the time
    float dt = mui_get_time() - state->last_time;
    state->last_time = mui_get_time();

    if (mui_is_inside_rectangle(mui_get_mouse_position(), place)) {
        if (mui_is_mouse_button_pressed(0)) {
            state->checked = !state->checked;
        }
        mui_move_towards(&(state->hover_t), 1, theme->animation_speed_to_hover, dt);
    } else {
        mui_move_towards(&(state->hover_t), 0, theme->animation_speed_to_normal, dt);
    }


    Mui_Rectangle area;     // a square for the mui_checkbox
    place = mui_cut_left(place, place.height, &area); // place of text

    Mui_Color bg = mui_interpolate_color(theme->bg, theme->bg_light, state->hover_t);
    Mui_Color border_color = mui_interpolate_color(theme->border, theme->primary, state->hover_t);
    Mui_Color text_color = mui_interpolate_color(theme->text_muted, theme->text, state->hover_t);

    area = mui_shrink(area, 2.0f);
    mui_draw_rectangle_rounded(area, theme->corner_radius, bg);
    mui_draw_rectangle_rounded_lines(mui_shrink(area, theme->border_thickness * 0.5f), theme->corner_radius, border_color, theme->border_thickness);

    float inset = area.height/4;

    if (state->checked) {
        area = mui_shrink(area, area.height/4);
        mui_draw_rectangle_rounded(area, theme->corner_radius - inset/2, text_color);
    }

    Mui_Vector2 position;
    position.x = place.x + inset;
    position.y = place.y + place.height / 2 - theme->font_size / 2;
    size_t l = mui_text_len(text, strlen(text));
    mui_draw_text_line(theme->font, position, 0, theme->font_size, text, text_color, 0, l);
}

float mui_simple_slider(Mui_Slider_State *state, bool vertical, Mui_Rectangle place) {

    Mui_Theme *theme = state->theme;
    if (theme == NULL) theme = &mui_protos_theme_g;

    // Update the time
    float dt = mui_get_time() - state->last_time;
    state->last_time = mui_get_time();
    Mui_Vector2 mpos = mui_get_mouse_position();

    if (mui_is_inside_rectangle(mpos, place)) {
        mui_move_towards(&(state->hover_t), 1, theme->animation_speed_to_hover, dt);
        if (mui_is_mouse_button_pressed(0)) {
            state->grabbed = true;
        }
    } else {
        mui_move_towards(&(state->hover_t), 0, theme->animation_speed_to_normal, dt);
    }

    if (mui_is_mouse_button_up(0)) {state->grabbed = false;}

    if (state->grabbed) {

        float s;
        if (vertical) s = fmin(fmax(0.0f, (mpos.y - place.y) / place.height), 1.0f);
        else          s = fmin(fmax(0.0f, (mpos.x - place.x) / place.width), 1.0f);
        state->value = s;
        mui_move_towards(&(state->hover_t), 1, theme->animation_speed_to_hover, dt);
    }

    Mui_Color bg = mui_interpolate_color(theme->bg, theme->bg_light, state->hover_t);
    Mui_Color border_color = mui_interpolate_color(theme->border, theme->primary, state->hover_t);
    Mui_Color text_color = mui_interpolate_color(theme->text_muted, theme->text, state->hover_t);

    if (!vertical) { // horizontal
        // [..]...........
        // ...........[..]
        // 0           -> place at 0
        // palce.width -> place at place.width - theme.slider
        float w = place.width - theme->slider_wagon_width;

        //rail
        Mui_Rectangle rail = mui_cut_top(place, place.height*0.5f - 0.5f*theme->slider_thickness, NULL);
        rail = mui_cut_bot(rail, place.height*0.5f - 0.5f*theme->slider_thickness, NULL);
        rail = mui_cut_left(rail, theme->slider_wagon_width*0.5f, NULL);
        rail = mui_cut_right(rail, theme->slider_wagon_width*0.5f, NULL);
        mui_draw_rectangle_rounded(rail, theme->slider_thickness*0.5f, text_color);

        Mui_Rectangle wagon;
        wagon.x = state->value * w + place.x;
        wagon.y = place.y + 0.5f*place.height - 0.5f*theme->slider_wagon_height;
        wagon.width = theme->slider_wagon_width;
        wagon.height = theme->slider_wagon_height;

        mui_draw_rectangle_rounded(wagon, theme->slider_wagon_corner_radius, bg);
        mui_draw_rectangle_rounded_lines(wagon, theme->slider_wagon_corner_radius, border_color, theme->slider_wagon_border_thickness);
    } else { // verical
        float w = place.height - theme->slider_wagon_width;

        //rail
        Mui_Rectangle rail = mui_cut_left(place, place.width * 0.5f - 0.5f * theme->slider_thickness, NULL);
        rail = mui_cut_right(rail, place.width * 0.5f - 0.5f * theme->slider_thickness, NULL);
        rail = mui_cut_top(rail, theme->slider_wagon_width * 0.5f, NULL);
        rail = mui_cut_bot(rail, theme->slider_wagon_width * 0.5f, NULL);
        mui_draw_rectangle_rounded(rail, theme->slider_thickness * 0.5f, text_color);

        Mui_Rectangle wagon;
        wagon.x = place.x + 0.5f * place.width - 0.5f * theme->slider_wagon_height;
        wagon.y = state->value * w + place.y;
        wagon.width = theme->slider_wagon_height;
        wagon.height = theme->slider_wagon_width;

        mui_draw_rectangle_rounded(wagon, theme->slider_wagon_corner_radius, bg);
        mui_draw_rectangle_rounded_lines(wagon, theme->slider_wagon_corner_radius, border_color, theme->slider_wagon_border_thickness);
    }
    return state->value;
}


bool mui_button(Mui_Button_State *state, const char* text, Mui_Rectangle place) {

    Mui_Theme *theme = state->theme;
    if (theme == NULL) {
        theme = &mui_protos_theme_g;
    }

    // Update the time
    float dt = mui_get_time() - state->last_time;
    state->last_time = mui_get_time();

    bool returnstate = false;

    if (mui_is_inside_rectangle(mui_get_mouse_position(), place)) {
        if (mui_is_mouse_button_pressed(0)) {
            returnstate = true;
        }
        mui_move_towards(&(state->hover_t), 1, theme->animation_speed_to_hover, dt);
    } else {
        mui_move_towards(&(state->hover_t), 0, theme->animation_speed_to_normal, dt);
    }

    Mui_Color bg;

    if (theme->border_thickness > 0) {
        bg = mui_interpolate_color(theme->bg, theme->bg_light, state->hover_t);
        mui_draw_rectangle_rounded(place, theme->corner_radius, bg);
        place = mui_shrink(place, theme->border_thickness);
    }

    bg = mui_interpolate_color(theme->bg, theme->primary_dark, state->hover_t);
    mui_draw_rectangle_rounded(place, theme->corner_radius - 0.25f * theme->corner_radius, bg);

    Mui_Color text_color = mui_interpolate_color(theme->text, theme->primary, state->hover_t);

    float offset = theme->font_size / 4;
    Mui_Vector2 position;
    position.x = place.x + offset;
    position.y = place.y + place.height / 2 - theme->font_size / 2;
    size_t l = mui_text_len(text, strlen(text));
    mui_draw_text_line(theme->label_font, position, 0, theme->font_size, text, text_color, 0, l);
    return returnstate;
}

/*
void mui_textinput_multiline(Mui_Textinput_Multiline_State *state, const char *hint, Mui_Rectangle place) {


    Mui_Theme *theme = state->theme;
    if (theme == NULL) {
        theme = &mui_protos_theme_g;
    }

    if (mui_is_mouse_button_pressed(0)) {
        if (mui_is_inside_rectangle(mui_get_mouse_position(), place)) {
            state->active = true;
        } else {
            state->active = false;
        }
    }

    if (state->buffer.count < 1) {
        utiiii_da_append(&(state->buffer), '\0');
    }

    if (state->active) {
        int unicode_char = mui_get_char_pressed();
        if (mui_is_key_pressed(MUI_KEY_ENTER) || mui_is_key_pressed_repeat(MUI_KEY_ENTER)) {unicode_char = '\n';}
        if (unicode_char != 0) {
            unsigned char c = unicode_char & 0xff; // TODO make it better

            // add at the end and then swap
            utiiii_da_append(&(state->buffer), '\0');
            assert(state->cursor < state->buffer.count-1);

            for( size_t i = state->buffer.count-2; i > state->cursor; i--) {
                state->buffer.items[i] = state->buffer.items[i-1];
            }

            printf("Input %c\n", unicode_char & 0xff);
            state->buffer.items[state->cursor] = c;
            state->cursor++;
        }

        if (mui_is_key_pressed(MUI_KEY_LEFT) || mui_is_key_pressed_repeat(MUI_KEY_LEFT)) {
            if (state->cursor != 0) state->cursor--;
        }

        if (mui_is_key_pressed(MUI_KEY_RIGHT) || mui_is_key_pressed_repeat(MUI_KEY_RIGHT)) {
            if (state->cursor < state->buffer.count-1) state->cursor++;
        }

        if (mui_is_key_pressed(MUI_KEY_BACKSPACE) || mui_is_key_pressed_repeat(MUI_KEY_BACKSPACE)) {
            if (state->buffer.count > 1 && state->cursor > 0) {
                for (size_t i = state->cursor; i < state->buffer.count-1; i++) {
                    state->buffer.items[i-1] = state->buffer.items[i];
                }
                state->cursor--;
                state->buffer.count--;
            }
        }
    }


    mui_draw_rectangle_rounded(place, theme->corner_radius, theme->border_color);
    place = mui_shrink(place, theme->border_thickness);
    mui_draw_rectangle_rounded(place, theme->corner_radius, theme->textinput_background_color);

    float text_offset_left = theme->textinput_text_size*0.1f;
    float text_offset_top = theme->textinput_text_size*0.1f;

    // cursor
    Mui_Rectangle rect_cursor;
    rect_cursor.x = place.x + text_offset_left;
    rect_cursor.y = place.y + text_offset_top;
    rect_cursor.width = 3;
    rect_cursor.height = theme->textinput_text_size;

    if (state->buffer.count > 1 || state->active) {
        const float line_spacing = 1.1f;

        int line_nr = 0;
        char* line_start = state->buffer.items;

        Mui_Vector2 line_size;
        line_size.x = 0.0f;
        line_size.y = 0.0f;


        for (size_t i = 0; i < state->buffer.count; i++) {
            char* buf = &state->buffer.items[i];
            line_size = mui_measure_text(theme->textinput_font, line_start, theme->textinput_text_size, 0.1f, 0, buf-line_start);

            // draw it
            if (line_size.x >= place.width - text_offset_left || *buf == '\n' || i==state->buffer.count-1) {
                char* orig = buf;
                if(i != state->buffer.count-1 || *buf == '\n') {
                    buf--; // we overshot, so we do backtracking
                }

                Mui_Vector2 position;
                position.x = place.x + text_offset_left;
                position.y = place.y + text_offset_top + line_nr*theme->textinput_text_size*line_spacing;
                size_t l = buf-line_start;
                mui_draw_text_line(theme->textinput_font, position, 0.1, theme->textinput_text_size, line_start, theme->textinput_text_color, 0, l);

                if (*orig == '\n') buf += 2; // skip new line and backtracking
                line_start = buf;
                line_nr++;
            }

            // check for cursor
            if (i == state->cursor) {
                if (state->cursor == state->buffer.count-1) line_nr--;
                rect_cursor.x += line_size.x;
                rect_cursor.y += line_nr*theme->textinput_text_size*line_spacing;
            }
        }

    } else {
        //hint text
        size_t l = mui_text_len(hint, strlen(hint));
        Mui_Vector2 position;
        position.x = place.x + text_offset_left;
        position.y = place.y + text_offset_top;
        mui_draw_text_line(theme->textinput_font, position, 0.1f, theme->textinput_text_size, hint, theme->textinput_text_color, 0, l);
    }

    // draw cursor
    if (state->active) {
        mui_draw_rectangle(rect_cursor, theme->textinput_text_color);
    }
}*/

char* next_occurence_or_null(char* text, size_t start, char c) {
    while(text[start] != 0) {
        start++;
        if(text[start] == c) return &(text[start]);
    }
    return NULL; // reached the end
}


size_t _internal_get_cursor_by_position(Mui_Vector2 pos, char* text, size_t* start_cursor, size_t* end_cursor, size_t n_lines, struct Mui_Font* font, float font_size, Mui_Rectangle place)
{
    int line_clicked = (pos.y - place.y) / font_size;

    if (line_clicked < 0) line_clicked = 0;
    else if (line_clicked >= (int)n_lines) line_clicked = n_lines -1;

    size_t line_start = start_cursor[line_clicked];
    size_t line_end = end_cursor[line_clicked];

    size_t cursor_offset = 0;
    for (int i = 0; i < (int)(line_end - line_start); i++) {
        Mui_Vector2 s = mui_measure_text(font, text, font_size, 0.1f, line_start, line_start + i);
        if (s.x + place.x > pos.x) {
            cursor_offset = max(0, i);
            break;
        }
        cursor_offset = max(0, i+1);
    }

    return line_start + cursor_offset;
}

// for now we can only have one of these text_selectable elements I guess,
// since we have one global mouse_down_selectable_text
static bool mouse_down_selectable_text;
size_t mouse_down_pivot_cursor;
void mui_text_selectable(char* text, size_t *selector1, size_t *selector2, Mui_Rectangle place) {
    Mui_Theme *theme = &mui_protos_theme_g;
    float font_size = theme->textinput_text_size;
    Mui_Color text_color = theme->text;
    struct Mui_Font* font = theme->textinput_font;

    size_t total_length = strlen(text);
    mui_draw_rectangle(place, theme->bg_light);
    place = mui_shrink(place, 2.0f * 2);

    //
    // segent into text lines
    //
    #define MAX_LINES 50
    size_t start_cursor[MAX_LINES];
    size_t end_cursor[MAX_LINES];
    size_t n_lines;
    // TODO: out of bounds checks
    int line_nr = 0;
    char* prev = text;
    char* next = next_occurence_or_null(prev, 0, '\n');
    while (next != NULL) {
        size_t line_start = prev - text;
        size_t line_end = line_start + next - prev;
        start_cursor[line_nr] = line_start;
        end_cursor[line_nr] = line_end; // include newline for now
        prev = next + 1; // skip new lines
        next = next_occurence_or_null(next, 0, '\n');
        line_nr++;
    }
    if ((size_t)(prev - text) < total_length - 1) {
        start_cursor[line_nr] = prev - text;
        end_cursor[line_nr] = total_length;
        line_nr++;
    }

    n_lines = line_nr;

    //
    // input handling
    //
    if (mui_is_inside_rectangle(mui_get_mouse_position(), place)) {
        mui_set_mouse_cursor(MUI_MOUSE_CURSOR_IBEAM);
    } else {
        mui_set_mouse_cursor(MUI_MOUSE_CURSOR_DEFAULT);
    }
    if (mui_is_mouse_button_pressed(0) && mui_is_inside_rectangle(mui_get_mouse_position(), place)) {
        mouse_down_selectable_text = true;
        Mui_Vector2 mouse = mui_get_mouse_position();
        size_t new_cursor = _internal_get_cursor_by_position(mouse, text, start_cursor, end_cursor, n_lines, font, font_size, place);
        *selector1 = new_cursor;
        *selector2 = new_cursor;
        mouse_down_pivot_cursor = new_cursor;
    }
    // TODO: also handle other events that should cancel this thing, like mouse goes out of window.
    if (mui_is_mouse_button_up(0)) mouse_down_selectable_text = false;


    if (mouse_down_selectable_text) {
        Mui_Vector2 mouse = mui_get_mouse_position();
        size_t new_cursor = _internal_get_cursor_by_position(mouse, text, start_cursor, end_cursor, n_lines, font, font_size, place);
        *selector1 = mouse_down_pivot_cursor;
        *selector2 = mouse_down_pivot_cursor;
        if (new_cursor > mouse_down_pivot_cursor) {
            // to the right
            *selector2 = new_cursor;
        }
        if (new_cursor < mouse_down_pivot_cursor) {
            // to the left
            *selector1 = new_cursor;
        }
    }

    // clamp selectors
    *selector1 = max(*selector1, 0);
    *selector2 = min(*selector2, total_length-1);

    //
    // find stuff about selections
    //
    float selection_start_x;
    float selection_end_x;
    size_t selection_start_line;
    size_t selection_end_line;
    size_t selection_start_line_offset;
    size_t selection_end_line_offset;
    for (size_t i = 0; i < n_lines; i++) {
        size_t line_start = start_cursor[i];
        size_t line_end = end_cursor[i];

        if(*selector1 >= line_start && *selector1 <= line_end) {
            selection_start_line = i;
            selection_start_line_offset = *selector1 - line_start;
            Mui_Vector2 s1 = mui_measure_text(font, text, font_size, 0.1f, line_start, line_start + selection_start_line_offset);
            selection_start_x = s1.x;
        }

        if(*selector2 >= line_start && *selector2 <= line_end) {
            selection_end_line = i;
            selection_end_line_offset = *selector2 - line_start;
            Mui_Vector2 s2 = mui_measure_text(font, text, font_size, 0.1f, line_start, line_start + selection_end_line_offset);
            selection_end_x = s2.x;
        }
    }

    //
    // drawing selection
    //
    for (size_t i = 0; i < n_lines; i++) {
        size_t line_start = start_cursor[i];
        size_t line_end = end_cursor[i];

        if (i >= selection_start_line && i <= selection_end_line) {
            float end_x = selection_end_x;
            if (selection_end_line != i) {
                end_x = mui_measure_text(font, text, font_size, 0.1f, line_start, line_end).x;
            }

            float start_x = selection_start_x;
            if (selection_start_line != i) {
                start_x = 0;
            }

            Mui_Rectangle selected;
            selected.x = place.x + start_x;
            selected.y = place.y + font_size * i;
            selected.width = end_x - start_x;
            selected.height = font_size;
            mui_draw_rectangle(selected, MUI_BLUE);
        }
    }

    //
    // drawing text
    //
    for (size_t i = 0; i < n_lines; i++) {
        size_t line_start = start_cursor[i];
        size_t line_end = end_cursor[i];
        Mui_Vector2 pos = (Mui_Vector2){place.x, place.y + font_size * i};
        mui_draw_text_line(font, pos, 0.1f, font_size, text, text_color, line_start, line_end);
    }
}

void mui_textinput(Mui_Textinput_State *state, const char *hint, Mui_Rectangle place) {

    Mui_Theme *theme = state->theme;
    if (theme == NULL) {
        theme = &mui_protos_theme_g;
    }

    if (mui_is_mouse_button_pressed(0)) {
        if (mui_is_inside_rectangle(mui_get_mouse_position(), place)) {
            state->active = true;
        } else {
            state->active = false;
        }
    }

    if (state->active) {
        int unicode_char = mui_get_char_pressed();
        if (unicode_char != 0) {
            unsigned char c = unicode_char & 0xff; // TODO make it better
            printf("Input %c\n", unicode_char & 0xff);
            if (state->count < MUI_TEXTINPUT_CAP) {
                state->buf[state->count] = c;
                state->buf[state->count+1] = 0;
                state->count++;
            }
        }

        if (mui_is_key_pressed(MUI_KEY_BACKSPACE) || mui_is_key_pressed_repeat(MUI_KEY_BACKSPACE)) {
            if (state->count > 0) {
                state->count--;
                state->buf[state->count] = 0;
            }
        }
    }


    mui_draw_rectangle_rounded(place, theme->corner_radius, theme->border);
    place = mui_shrink(place, theme->border_thickness);
    mui_draw_rectangle_rounded(place, theme->corner_radius, theme->bg_light);

    float text_offset_left = place.height/2 - theme->font_size/2;


    if (state->active) {
        // cursor
        Mui_Vector2 offset = mui_measure_text(theme->textinput_font, state->buf, theme->textinput_text_size, 0.1f, 0, state->count-1);
        int pos = place.x + text_offset_left + offset.x;
        Mui_Rectangle rc;
        rc.x = pos;
        rc.y = place.y + place.height / 2 - theme->textinput_text_size / 2;
        rc.width = 3;
        rc.height = theme->textinput_text_size;
        mui_draw_rectangle(rc, theme->text);
    }


    if (state->count != 0 || state->active) {
        size_t l = mui_text_len(state->buf, strlen(state->buf));
        Mui_Vector2 position;
        position.x = place.x + text_offset_left;
        position.y = place.y + place.height / 2 - theme->textinput_text_size / 2;
        mui_draw_text_line(theme->textinput_font, position, 0.1, theme->textinput_text_size, state->buf, theme->text, 0, l);
    } else {
        //hint text
        size_t l = mui_text_len(hint, strlen(hint));
        Mui_Vector2 position;
        position.x = place.x + text_offset_left;
        position.y = place.y + place.height / 2 - theme->textinput_text_size / 2;
        mui_draw_text_line(theme->textinput_font, position, 0.1, theme->textinput_text_size, hint, theme->text_muted, 0, l);
    }
}


//
// Some easing functions: See https://easings.net/
//
#ifndef M_PI
#define M_PI 3.14159265358979323846  /* pi from math.h */
#endif
float mui_ease_in_quad(float t)      { return t * t; }
float mui_ease_out_quad(float t)     { return 1 - (1 - t) * (1 - t); }
float mui_ease_in_out_quad(float t)  { return t < 0.5f ? 2 * t * t: 1 - 2*(1 - t)*(1 - t); }
float mui_ease_in_cubic(float t)     { return t * t * t; }
float mui_ease_out_cubic(float t)    { return 1 - (1 - t) * (1 - t) * (1 - t); }
float mui_ease_in_out_cubic(float t) { return t < 0.5f ? 4 * t * t * t : 1 - 4*(1 - t)*(1 - t)*(1 - t); }
float mui_ease_in_sin(float t)       { return 1 - cosf((t * M_PI) * 0.5f); }
float mui_ease_out_sin(float t)      { return sinf((t * M_PI) *0.5f); }
float mui_ease_in_out_sin(float t)   { return - (cosf((t * M_PI) / 2) - 1) * 0.5f; }