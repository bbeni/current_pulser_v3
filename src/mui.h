// Copyright (C) 2026 Benjamin Froelich
// This file is part of https://github.com/bbeni/impedancer
// For conditions of distribution and use, see copyright notice in project root.
/* Mui library

Copyright 2024 - 2026 Benjamin Froelich

Platform independent user interface.
Files needed:
    -mui.h
    -mui.c
    -mui_platform_raylib.c (or whatever platform we are on)
*/
#ifndef MUI_H_
#define MUI_H_

#include "math.h"
#include "stddef.h"
#include "stdbool.h"
#include "stdint.h"


// primitives
typedef struct Mui_Color {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} Mui_Color;

typedef struct Mui_Rectangle {
    float x;
    float y;
    float width;
    float height;
} Mui_Rectangle;

typedef struct Mui_Vector2 {
    float x;
    float y;
} Mui_Vector2;

typedef struct Mui_Image {
    int id;
} Mui_Image;


//opaque needs be implemented in platform
struct Mui_Font;

typedef struct {
    Mui_Color bg_dark;
    Mui_Color bg;
    Mui_Color bg_light;

    Mui_Color text;
    Mui_Color text_muted;

    Mui_Color border;
    Mui_Color primary;
    Mui_Color primary_dark;

    float font_size;
    float label_text_size;
    float textinput_text_size;

    struct Mui_Font *font;
    struct Mui_Font *label_font;
    struct Mui_Font *textinput_font;

    float corner_radius;
    float border_thickness;

    float slider_thickness;
    float slider_wagon_width;
    float slider_wagon_height;
    float slider_wagon_corner_radius;
    float slider_wagon_border_thickness;

    float animation_speed_to_hover;
    float animation_speed_to_normal;

} Mui_Theme;

// default theme
extern Mui_Theme mui_protos_theme_g;
extern Mui_Theme mui_protos_theme_dark_g;
extern Mui_Theme mui_protos_theme_light_g;

#define MAX_FONTS_LOADED 128
extern struct Mui_Font *mui_font_catalog_g[MAX_FONTS_LOADED];
extern size_t mui_font_catalog_length_g;

typedef struct {
    float hover_t; // from 0 to 1 representing the hover state (animation)
    float last_time;

    Mui_Theme *theme; // can be NULL
} Mui_Button_State;

typedef struct {
    float hover_t; // from 0 to 1 representing the hover state (animation)
    float last_time;
    bool checked;

    Mui_Theme *theme; // can be NULL
} Mui_Checkbox_State;


typedef struct {
    float hover_t; // from 0 to 1 representing the hover state (animation)
    float last_time;
    float value; // from 0 to 1 (left to right)

    bool grabbed;

    Mui_Theme *theme; // can be NULL
} Mui_Slider_State;

typedef struct {
    float hover_t; // from 0 to 1 representing the hover state (animation)
    float last_time;
    bool open;
    Mui_Theme *theme; // can be NULL
} Mui_Collapsable_Section_State;


#ifndef MUI_TEXTINPUT_CAP
#define MUI_TEXTINPUT_CAP 511
#endif
typedef struct {
    char buf[MUI_TEXTINPUT_CAP + 1];
    size_t count;
    size_t cursor;
    bool active; // wheter it was clicked and it is writable

    Mui_Theme *theme;
} Mui_Textinput_State;


struct Text_Buffer {
    char* items;
    size_t capacity;
    size_t count;
};

typedef struct Mui_Textinput_Multiline_State {
    struct Text_Buffer buffer;
    size_t cursor;
    bool active; // wheter it was clicked and it is writable

    Mui_Theme *theme;
} Mui_Textinput_Multiline_State;

//
// state initialization
//

Mui_Button_State mui_button_state();
Mui_Checkbox_State mui_checkbox_state();
Mui_Slider_State mui_slider_state();
Mui_Collapsable_Section_State mui_collapsable_state();
Mui_Textinput_State mui_textinput_state();

//
// mui.h utility API
//

Mui_Color mui_interpolate_color(Mui_Color a, Mui_Color b, float t);
void mui_move_towards(float *x, float target, float speed, float dt);
Mui_Rectangle mui_rectangle(float x, float y, float width, float height);
Mui_Rectangle mui_shrink(Mui_Rectangle r, float amount);
Mui_Rectangle mui_cut_left(Mui_Rectangle r, float amount, Mui_Rectangle *out_left);
Mui_Rectangle mui_cut_right(Mui_Rectangle r, float amount, Mui_Rectangle *out_right);
Mui_Rectangle mui_cut_top(Mui_Rectangle r, float amount, Mui_Rectangle *out_top);
Mui_Rectangle mui_cut_bot(Mui_Rectangle r, float amount, Mui_Rectangle *out_bot);
Mui_Vector2 mui_center_of_rectangle(Mui_Rectangle rectangle);
void mui_grid_22(Mui_Rectangle r, float factor_x, float factor_y, Mui_Rectangle *out_11, Mui_Rectangle *out_12, Mui_Rectangle *out_21, Mui_Rectangle *out_22);
bool mui_is_inside_rectangle(Mui_Vector2, Mui_Rectangle);
void mui_center_rectangle_inside_rectangle(Mui_Rectangle* inner, Mui_Rectangle outer);

//
// mui elements API
//

bool mui_load_ttf_font_for_theme(const char *font_file, Mui_Theme* theme);
// returns the rest of the space left ( to the left or right ) as a rectangle
Mui_Rectangle mui_window_decoration(float height, bool window_movable, bool closeable, bool minimizable, bool maximizable, bool to_the_right, Mui_Rectangle window_rect);
bool mui_button(Mui_Button_State *state, const char* text, Mui_Rectangle place);
void mui_checkbox(Mui_Checkbox_State *state, const char *text, Mui_Rectangle place);
void mui_label(Mui_Theme *theme, char *text, Mui_Rectangle place);
float mui_simple_slider(Mui_Slider_State *state, bool vertical, Mui_Rectangle place);
void mui_textinput(Mui_Textinput_State *state, const char *hint, Mui_Rectangle place);
void mui_textinput_multiline(Mui_Textinput_Multiline_State *state, const char *hint, Mui_Rectangle place);
void mui_text_selectable(char* text, size_t *selector1, size_t *selector2, Mui_Rectangle place);
bool mui_collapsable_section(Mui_Collapsable_Section_State *state, char* text, Mui_Rectangle place);

//
// window API platform
//

typedef enum {
    MUI_WINDOW_FULLSCREEN = 0x1,
    MUI_WINDOW_BORDERLESS = 0x2,
    MUI_WINDOW_RESIZEABLE = 0x4,
    MUI_WINDOW_HIDDEN = 0x8,
    MUI_WINDOW_MINIMIZED = 0x10,
    MUI_WINDOW_MAXIMIZED = 0x20,
    MUI_WINDOW_FOCUSED = 0x40,
    MUI_WINDOW_UNDECORATED = 0x80,
} MUI_WINDOW_FLAGS;

// init themes with chrma value for the backgorund (okhcl space). chroma_bg in [0, 0.4] lower is no color.
// if ttf_file_name is NULL take latest fonts.
void mui_init_themes(float chroma_bg, float bg_hue, bool dark, char* ttf_file_name);
uint8_t mui_open_window(int w, int h, int pos_x, int pos_y, char* title, float opacity, MUI_WINDOW_FLAGS flags, Mui_Image* icon);
uint8_t mui_get_active_window_id();
int mui_screen_width();
int mui_screen_height();
bool mui_window_should_close();
void mui_window_restore();
void mui_window_maximize();
void mui_window_minimize();
bool mui_is_window_maximized();
Mui_Vector2 mui_window_get_position();
void mui_window_set_position(int x, int y);
void mui_window_set_size(int width, int height);



void mui_clear_background(Mui_Color color, Mui_Image* image);
void mui_begin_drawing();
void mui_end_drawing();
void mui_close_window();

size_t mui_text_len(const char* text, size_t size);

double mui_get_time();
void mui_update_core();

//
// utility API platform binding
//

double mui_get_time_now();
float mui_get_frame_time_now();
Mui_Vector2 mui_get_mouse_position_now();

//
// drawing primitives API platform
//

void mui_draw_pixel(Mui_Vector2 pos, Mui_Color color);
void mui_draw_circle(Mui_Vector2 pos, float radius, Mui_Color color);
void mui_draw_circle_lines(Mui_Vector2 center, float radius, Mui_Color color, float thickness);
// start and end in degrees
void mui_draw_arc_lines(Mui_Vector2 center, float radius, float start_angle, float end_angle, Mui_Color color, float thickness);
void mui_draw_line(float start_x, float start_y, float end_x, float end_y, float thickness, Mui_Color color);
void mui_draw_rectangle(Mui_Rectangle rect, Mui_Color color);
void mui_draw_rectangle_rounded(Mui_Rectangle rect, float corner_radius, Mui_Color color);
void mui_draw_rectangle_lines(Mui_Rectangle rect, Mui_Color color, float thickness);
void mui_draw_rectangle_rounded_lines(Mui_Rectangle rect, float corner_radius, Mui_Color color, float thickness);

Mui_Vector2 mui_measure_text(struct Mui_Font* font, const char *text, float font_size, float spacing, size_t start, size_t end);
struct Mui_Font *mui_load_font_ttf(void* ttf_data, int ttf_data_size, float font_size);
void mui_draw_text_line(struct Mui_Font* font, Mui_Vector2 pos, float letter_space, float letter_size, const char* text, Mui_Color color, size_t start, size_t end);

#define MUI_LIGHTGRAY  (Mui_Color){ 200, 200, 200, 255 }   // Light Gray
#define MUI_GRAY       (Mui_Color){ 130, 130, 130, 255 }   // Gray
#define MUI_DARKGRAY   (Mui_Color){ 80, 80, 80, 255 }      // Dark Gray
#define MUI_YELLOW     (Mui_Color){ 253, 249, 0, 255 }     // Yellow
#define MUI_GOLD       (Mui_Color){ 255, 203, 0, 255 }     // Gold
#define MUI_ORANGE     (Mui_Color){ 255, 161, 0, 255 }     // Orange
#define MUI_PINK       (Mui_Color){ 255, 109, 194, 255 }   // Pink
#define MUI_RED        (Mui_Color){ 230, 41, 55, 255 }     // Red
#define MUI_MAROON     (Mui_Color){ 190, 33, 55, 255 }     // Maroon
#define MUI_GREEN      (Mui_Color){ 0, 228, 48, 255 }      // Green
#define MUI_LIME       (Mui_Color){ 0, 158, 47, 255 }      // Lime
#define MUI_DARKGREEN  (Mui_Color){ 0, 117, 44, 255 }      // Dark Green
#define MUI_SKYBLUE    (Mui_Color){ 102, 191, 255, 255 }   // Sky Blue
#define MUI_BLUE       (Mui_Color){ 0, 121, 241, 255 }     // Blue
#define MUI_DARKBLUE   (Mui_Color){ 0, 82, 172, 255 }      // Dark Blue
#define MUI_PURPLE     (Mui_Color){ 200, 122, 255, 255 }   // Purple
#define MUI_VIOLET     (Mui_Color){ 135, 60, 190, 255 }    // Violet
#define MUI_DARKPURPLE (Mui_Color){ 112, 31, 126, 255 }    // Dark Purple
#define MUI_BEIGE      (Mui_Color){ 211, 176, 131, 255 }   // Beige
#define MUI_BROWN      (Mui_Color){ 127, 106, 79, 255 }    // Brown
#define MUI_DARKBROWN  (Mui_Color){ 76, 63, 47, 255 }      // Dark Brown
#define MUI_WHITE      (Mui_Color){ 255, 255, 255, 255 }   // White
#define MUI_BLACK      (Mui_Color){ 0, 0, 0, 255 }         // Black
#define MUI_BLANK      (Mui_Color){ 0, 0, 0, 0 }           // Blank (Transparent)
#define MUI_MAGENTA    (Mui_Color){ 255, 0, 255, 255 }     // Magenta
#define MUI_RAYWHITE   (Mui_Color){ 245, 245, 245, 255 }   // My own White (raylib logo)



// FROM RAYLIB.
// Keyboard keys (US keyboard layout)
// NOTE: Use GetKeyPressed() to allow redefining
// required keys for alternative layouts
typedef enum {
    MUI_KEY_NULL            = 0,        // Key: NULL, used for no key pressed
    // Alphanumeric keys
    MUI_KEY_APOSTROPHE      = 39,       // Key: '
    MUI_KEY_COMMA           = 44,       // Key: ,
    MUI_KEY_MINUS           = 45,       // Key: -
    MUI_KEY_PERIOD          = 46,       // Key: .
    MUI_KEY_SLASH           = 47,       // Key: /
    MUI_KEY_ZERO            = 48,       // Key: 0
    MUI_KEY_ONE             = 49,       // Key: 1
    MUI_KEY_TWO             = 50,       // Key: 2
    MUI_KEY_THREE           = 51,       // Key: 3
    MUI_KEY_FOUR            = 52,       // Key: 4
    MUI_KEY_FIVE            = 53,       // Key: 5
    MUI_KEY_SIX             = 54,       // Key: 6
    MUI_KEY_SEVEN           = 55,       // Key: 7
    MUI_KEY_EIGHT           = 56,       // Key: 8
    MUI_KEY_NINE            = 57,       // Key: 9
    MUI_KEY_SEMICOLON       = 59,       // Key: ;
    MUI_KEY_EQUAL           = 61,       // Key: =
    MUI_KEY_A               = 65,       // Key: A | a
    MUI_KEY_B               = 66,       // Key: B | b
    MUI_KEY_C               = 67,       // Key: C | c
    MUI_KEY_D               = 68,       // Key: D | d
    MUI_KEY_E               = 69,       // Key: E | e
    MUI_KEY_F               = 70,       // Key: F | f
    MUI_KEY_G               = 71,       // Key: G | g
    MUI_KEY_H               = 72,       // Key: H | h
    MUI_KEY_I               = 73,       // Key: I | i
    MUI_KEY_J               = 74,       // Key: J | j
    MUI_KEY_K               = 75,       // Key: K | k
    MUI_KEY_L               = 76,       // Key: L | l
    MUI_KEY_M               = 77,       // Key: M | m
    MUI_KEY_N               = 78,       // Key: N | n
    MUI_KEY_O               = 79,       // Key: O | o
    MUI_KEY_P               = 80,       // Key: P | p
    MUI_KEY_Q               = 81,       // Key: Q | q
    MUI_KEY_R               = 82,       // Key: R | r
    MUI_KEY_S               = 83,       // Key: S | s
    MUI_KEY_T               = 84,       // Key: T | t
    MUI_KEY_U               = 85,       // Key: U | u
    MUI_KEY_V               = 86,       // Key: V | v
    MUI_KEY_W               = 87,       // Key: W | w
    MUI_KEY_X               = 88,       // Key: X | x
    MUI_KEY_Y               = 89,       // Key: Y | y
    MUI_KEY_Z               = 90,       // Key: Z | z
    MUI_KEY_LEFT_BRACKET    = 91,       // Key: [
    MUI_KEY_BACKSLASH       = 92,       // Key: '\'
    MUI_KEY_RIGHT_BRACKET   = 93,       // Key: ]
    MUI_KEY_GRAVE           = 96,       // Key: `
    // Function keys
    MUI_KEY_SPACE           = 32,       // Key: Space
    MUI_KEY_ESCAPE          = 256,      // Key: Esc
    MUI_KEY_ENTER           = 257,      // Key: Enter
    MUI_KEY_TAB             = 258,      // Key: Tab
    MUI_KEY_BACKSPACE       = 259,      // Key: Backspace
    MUI_KEY_INSERT          = 260,      // Key: Ins
    MUI_KEY_DELETE          = 261,      // Key: Del
    MUI_KEY_RIGHT           = 262,      // Key: Cursor right
    MUI_KEY_LEFT            = 263,      // Key: Cursor left
    MUI_KEY_DOWN            = 264,      // Key: Cursor down
    MUI_KEY_UP              = 265,      // Key: Cursor up
    MUI_KEY_PAGE_UP         = 266,      // Key: Page up
    MUI_KEY_PAGE_DOWN       = 267,      // Key: Page down
    MUI_KEY_HOME            = 268,      // Key: Home
    MUI_KEY_END             = 269,      // Key: End
    MUI_KEY_CAPS_LOCK       = 280,      // Key: Caps lock
    MUI_KEY_SCROLL_LOCK     = 281,      // Key: Scroll down
    MUI_KEY_NUM_LOCK        = 282,      // Key: Num lock
    MUI_KEY_PRINT_SCREEN    = 283,      // Key: Print screen
    MUI_KEY_PAUSE           = 284,      // Key: Pause
    MUI_KEY_F1              = 290,      // Key: F1
    MUI_KEY_F2              = 291,      // Key: F2
    MUI_KEY_F3              = 292,      // Key: F3
    MUI_KEY_F4              = 293,      // Key: F4
    MUI_KEY_F5              = 294,      // Key: F5
    MUI_KEY_F6              = 295,      // Key: F6
    MUI_KEY_F7              = 296,      // Key: F7
    MUI_KEY_F8              = 297,      // Key: F8
    MUI_KEY_F9              = 298,      // Key: F9
    MUI_KEY_F10             = 299,      // Key: F10
    MUI_KEY_F11             = 300,      // Key: F11
    MUI_KEY_F12             = 301,      // Key: F12
    MUI_KEY_LEFT_SHIFT      = 340,      // Key: Shift left
    MUI_KEY_LEFT_CONTROL    = 341,      // Key: Control left
    MUI_KEY_LEFT_ALT        = 342,      // Key: Alt left
    MUI_KEY_LEFT_SUPER      = 343,      // Key: Super left
    MUI_KEY_RIGHT_SHIFT     = 344,      // Key: Shift right
    MUI_KEY_RIGHT_CONTROL   = 345,      // Key: Control right
    MUI_KEY_RIGHT_ALT       = 346,      // Key: Alt right
    MUI_KEY_RIGHT_SUPER     = 347,      // Key: Super right
    MUI_KEY_KB_MENU         = 348,      // Key: KB menu
    // Keypad keys
    MUI_KEY_KP_0            = 320,      // Key: Keypad 0
    MUI_KEY_KP_1            = 321,      // Key: Keypad 1
    MUI_KEY_KP_2            = 322,      // Key: Keypad 2
    MUI_KEY_KP_3            = 323,      // Key: Keypad 3
    MUI_KEY_KP_4            = 324,      // Key: Keypad 4
    MUI_KEY_KP_5            = 325,      // Key: Keypad 5
    MUI_KEY_KP_6            = 326,      // Key: Keypad 6
    MUI_KEY_KP_7            = 327,      // Key: Keypad 7
    MUI_KEY_KP_8            = 328,      // Key: Keypad 8
    MUI_KEY_KP_9            = 329,      // Key: Keypad 9
    MUI_KEY_KP_DECIMAL      = 330,      // Key: Keypad .
    MUI_KEY_KP_DIVIDE       = 331,      // Key: Keypad /
    MUI_KEY_KP_MULTIPLY     = 332,      // Key: Keypad *
    MUI_KEY_KP_SUBTRACT     = 333,      // Key: Keypad -
    MUI_KEY_KP_ADD          = 334,      // Key: Keypad +
    MUI_KEY_KP_ENTER        = 335,      // Key: Keypad Enter
    MUI_KEY_KP_EQUAL        = 336,      // Key: Keypad =
    // Android key buttons
    MUI_ANDDROID_KEY_BACK            = 4,        // Key: Android back button
    MUI_ANDDROID_KEY_MENU            = 5,        // Key: Android menu button
    MUI_ANDDROID_KEY_VOLUME_UP       = 24,       // Key: Android volume up button
    MUI_ANDDROID_KEY_VOLUME_DOWN     = 25        // Key: Android volume down button
} Mui_Keyboard_Key;

//
// input/output/controller API platform
//
bool mui_is_key_down(Mui_Keyboard_Key key);
bool mui_is_key_up(Mui_Keyboard_Key key);
bool mui_is_key_pressed(Mui_Keyboard_Key key);
bool mui_is_key_pressed_repeat(Mui_Keyboard_Key key);
int mui_get_char_pressed();
bool mui_is_mouse_button_pressed(int button);
bool mui_is_mouse_button_down(int button);
bool mui_is_mouse_button_up(int button);
bool mui_is_mouse_button_released(int button);

void mui_set_clipboard_text(char* text);
const char* mui_clipboard_text();
Mui_Vector2 mui_get_mouse_position();

// Mouse cursor
typedef enum {
    MUI_MOUSE_CURSOR_DEFAULT       = 0,     // Default pointer shape
    MUI_MOUSE_CURSOR_ARROW         = 1,     // Arrow shape
    MUI_MOUSE_CURSOR_IBEAM         = 2,     // Text writing cursor shape
    MUI_MOUSE_CURSOR_CROSSHAIR     = 3,     // Cross shape
    MUI_MOUSE_CURSOR_POINTING_HAND = 4,     // Pointing hand cursor
    MUI_MOUSE_CURSOR_RESIZE_EW     = 5,     // Horizontal resize/move arrow shape
    MUI_MOUSE_CURSOR_RESIZE_NS     = 6,     // Vertical resize/move arrow shape
    MUI_MOUSE_CURSOR_RESIZE_NWSE   = 7,     // Top-left to bottom-right diagonal resize/move arrow shape
    MUI_MOUSE_CURSOR_RESIZE_NESW   = 8,     // The top-right to bottom-left diagonal resize/move arrow shape
    MUI_MOUSE_CURSOR_RESIZE_ALL    = 9,     // The omnidirectional resize/move cursor shape
    MUI_MOUSE_CURSOR_NOT_ALLOWED   = 10     // The operation-not-allowed shape
} MUI_MOUSE_CURSOR_TYPES;

void mui_set_mouse_cursor(MUI_MOUSE_CURSOR_TYPES type);

//
// Some easing functions from easings.net
//

float mui_ease_in_quad(float t);
float mui_ease_out_quad(float t);
float mui_ease_in_out_quad(float t);
float mui_ease_in_cubic(float t);
float mui_ease_out_cubic(float t);
float mui_ease_in_out_cubic(float t);
float mui_ease_in_sin(float t);
float mui_ease_out_sin(float t);
float mui_ease_in_out_sin(float t);


#endif // MUI_H_