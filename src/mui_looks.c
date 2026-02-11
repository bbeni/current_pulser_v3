#include "mui.h"
#include <assert.h>
#include <stdio.h>

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
        .font_small_size = 20.0f,
        .label_text_size = 32.0f,
        .textinput_text_size = 24.0f,

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
        .font_small_size = 20.0f,
        .label_text_size = 32.0f,
        .textinput_text_size = 24.0f,

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
    mui_font_catalog_g[mui_font_catalog_length_g + 1] = mui_load_font_ttf(data, size, theme->font_small_size);
    mui_font_catalog_g[mui_font_catalog_length_g + 2] = mui_load_font_ttf(data, size, theme->label_text_size);
    mui_font_catalog_g[mui_font_catalog_length_g + 3] = mui_load_font_ttf(data, size, theme->textinput_text_size);
    theme->font =           mui_font_catalog_g[mui_font_catalog_length_g];
    theme->font_small =     mui_font_catalog_g[mui_font_catalog_length_g + 1];
    theme->label_font =     mui_font_catalog_g[mui_font_catalog_length_g + 2];
    theme->textinput_font = mui_font_catalog_g[mui_font_catalog_length_g + 3];
    mui_font_catalog_length_g += 4;

    return true;
}

void mui_load_latest_fonts_for_theme(Mui_Theme *theme) {
    assert(mui_font_catalog_length_g >= 4);
    theme->font =           mui_font_catalog_g[mui_font_catalog_length_g - 4];
    theme->font_small =     mui_font_catalog_g[mui_font_catalog_length_g - 3];
    theme->label_font =     mui_font_catalog_g[mui_font_catalog_length_g - 2];
    theme->textinput_font = mui_font_catalog_g[mui_font_catalog_length_g - 1];
}

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

float mui_linear_to_srgb(float x) {
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
        .r = (uint8_t)roundf(fmaxf(0, fminf(1, mui_linear_to_srgb(r_lin))) * 255),
        .g = (uint8_t)roundf(fmaxf(0, fminf(1, mui_linear_to_srgb(g_lin))) * 255),
        .b = (uint8_t)roundf(fmaxf(0, fminf(1, mui_linear_to_srgb(b_lin))) * 255),
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

Mui_Color mui_hsl_to_rgb(float h, float s, float l) {
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