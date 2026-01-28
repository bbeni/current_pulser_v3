// Copyright (C) 2026 Benjamin Froelich
// This file is part of https://github.com/bbeni/impedancer
// For conditions of distribution and use, see copyright notice in project root.
#include "mui.h"
#include "uti.h"

#include "raylib.h"
#include "rlgl.h"
#include "assert.h"
#include "string.h"
#include "stdlib.h"




uint8_t mui_open_window(int w, int h, int pos_x, int pos_y, char* title, float opacity, MUI_WINDOW_FLAGS flags, Mui_Image* icon) {

    // TODO: implement icon
    if (icon != NULL) {
        assert(false && "not implemented icon in mui_open_window()");
    }

    ConfigFlags raylib_flags =
        FLAG_VSYNC_HINT |
        //FLAG_WINDOW_HIGHDPI |
        FLAG_MSAA_4X_HINT |
        //FLAG_INTERLACED_HINT |
        0;

    if (opacity < 1.0f) raylib_flags &= FLAG_WINDOW_TRANSPARENT;
    if (0 < (flags & MUI_WINDOW_FULLSCREEN)) raylib_flags |= FLAG_FULLSCREEN_MODE;
    if (0 < (flags & MUI_WINDOW_BORDERLESS)) raylib_flags |= FLAG_BORDERLESS_WINDOWED_MODE;
    if (0 < (flags & MUI_WINDOW_RESIZEABLE)) raylib_flags |= FLAG_WINDOW_RESIZABLE;
    if (0 < (flags & MUI_WINDOW_HIDDEN)) raylib_flags |= FLAG_WINDOW_HIDDEN;
    if (0 < (flags & MUI_WINDOW_MINIMIZED)) raylib_flags |= FLAG_WINDOW_MINIMIZED;
    if (0 < (flags & MUI_WINDOW_MAXIMIZED)) {
        raylib_flags |= FLAG_WINDOW_MAXIMIZED;
    }
    if (0 < (flags & MUI_WINDOW_UNDECORATED)) raylib_flags |= FLAG_WINDOW_UNDECORATED;
    if (0 == (flags & MUI_WINDOW_FOCUSED)) raylib_flags |= FLAG_WINDOW_UNFOCUSED;

    SetConfigFlags(raylib_flags);
    InitWindow(w, h, title);
    SetWindowPosition(pos_x, pos_y);
    SetWindowOpacity(opacity);
    if (0 < (flags & MUI_WINDOW_MAXIMIZED)) {
        mui_window_maximize();
    }

    return 0;
}

uint8_t mui_get_active_window_id() {
    return 0;
}

int mui_screen_width()                  {return GetScreenWidth();}
int mui_screen_height()                 {return GetScreenHeight();}
bool mui_window_should_close()          {return WindowShouldClose();}
void mui_window_restore()               {RestoreWindow();}
void mui_window_maximize()              {MaximizeWindow();}
void mui_window_minimize()              {MinimizeWindow();}
bool mui_is_window_maximized()          {return IsWindowMaximized();}
Mui_Vector2 mui_window_get_position()   {
    Vector2 p = GetWindowPosition();
    Mui_Vector2 v;
    v.x = p.x;
    v.y = p.y;
    return v;
}
void mui_window_set_position(int x, int y)          {SetWindowPosition(x, y);}
void mui_window_set_size(int width, int height)     {SetWindowSize(width, height);}
void mui_set_clipboard_text(char* text) {SetClipboardText(text);}
const char* mui_clipboard_text()        {return GetClipboardText();}
void mui_set_mouse_cursor(MUI_MOUSE_CURSOR_TYPES type) { SetMouseCursor(type);};

void mui_clear_background(Mui_Color color, Mui_Image* image) {
    Color c;
    c.a = color.a;
    c.r = color.r;
    c.g = color.g;
    c.b = color.b;
    ClearBackground(c);
    assert(image == NULL && "TODO: implement image for background clearing");
}
void mui_begin_drawing()                {BeginDrawing();}
void mui_end_drawing()                  {EndDrawing();}
void mui_close_window()                 {CloseWindow();}

double mui_get_time_now()                   {return GetTime();}
float mui_get_frame_time_now()              {return GetFrameTime();}
Mui_Vector2 mui_get_mouse_position_now() {
    Vector2 p = GetMousePosition();
    Mui_Vector2 v;
    v.x = p.x;
    v.y = p.y;
    return v;
}


bool mui_is_key_down(Mui_Keyboard_Key key)           {return IsKeyDown(key);}
bool mui_is_key_up(Mui_Keyboard_Key key)             {return IsKeyUp(key);}
bool mui_is_key_pressed(Mui_Keyboard_Key key)        {return IsKeyPressed(key);}
bool mui_is_key_pressed_repeat(Mui_Keyboard_Key key) {return IsKeyPressedRepeat(key);}
int mui_get_char_pressed()                           {return GetCharPressed();}
bool mui_is_mouse_button_pressed(int button)         {return IsMouseButtonPressed(button);}
bool mui_is_mouse_button_released(int button)        {return IsMouseButtonReleased(button);}
bool mui_is_mouse_button_down(int button)            {return IsMouseButtonDown(button);}
bool mui_is_mouse_button_up(int button)              {return IsMouseButtonUp(button);}


//
// primitive drawing platform
//

#define RCOLOR(color) (Color){.r=(color).r, .g=(color).g, .b=(color).b, .a=(color).a}
#define RV2(vector) (Vector2){.x=(vector).x, .y=(vector).y}

void mui_draw_pixel(Mui_Vector2 pos, Mui_Color color) {
    DrawPixel(pos.x, pos.y, RCOLOR(color));
}

void mui_draw_circle(Mui_Vector2 pos, float radius, Mui_Color color) {
    DrawCircle(pos.x, pos.y, radius, RCOLOR(color));
}

void mui_draw_circle_lines(Mui_Vector2 center, float radius, Mui_Color color, float thickness) {
    //RLAPI void DrawCircleLines(int centerX, int centerY, float radius, Color color); // no thickness parameter.
    DrawRing((Vector2){center.x, center.y}, radius-thickness*0.5f, radius+thickness*0.5f, 0, 360, radius, RCOLOR(color));
}

// start and end in degrees
void mui_draw_arc_lines(Mui_Vector2 center, float radius, float start_angle, float end_angle, Mui_Color color, float thickness) {
    //RLAPI void DrawCircleLines(int centerX, int centerY, float radius, Color color); // no thickness parameter.
    DrawRing((Vector2){center.x, center.y}, radius-thickness*0.5f, radius+thickness*0.5f, start_angle, end_angle, 100, RCOLOR(color));
}

void mui_draw_line(float start_x, float start_y, float end_x, float end_y, float thickness, Mui_Color color) {
    DrawLineEx((Vector2){start_x, start_y}, (Vector2){end_x, end_y}, thickness, RCOLOR(color));
}

void mui_draw_rectangle(Mui_Rectangle rect, Mui_Color color) {
    DrawRectangle(rect.x, rect.y, rect.width, rect.height, RCOLOR(color));
}

float _raylib_roundedness(Mui_Rectangle rect, float radius) {
    float m = fmin(rect.width, rect.height);
    return radius * 2 / m;
}

void mui_draw_rectangle_rounded(Mui_Rectangle rect, float corner_radius, Mui_Color color) {
    float roundedness = _raylib_roundedness(rect, corner_radius);
    const int segments = 16;
    Rectangle r;
    r.x = rect.x;
    r.y = rect.y;
    r.width = rect.width;
    r.height = rect.height;
    DrawRectangleRounded(r, roundedness, segments, RCOLOR(color));
}

void mui_draw_rectangle_lines(Mui_Rectangle rect, Mui_Color color, float thickness) {
    Rectangle r;
    r.x = rect.x;
    r.y = rect.y;
    r.width = rect.width;
    r.height = rect.height;
    DrawRectangleLinesEx(r, thickness, RCOLOR(color));
}

void mui_draw_rectangle_rounded_lines(Mui_Rectangle rect, float corner_radius, Mui_Color color, float thickness) {
    float roundedness = _raylib_roundedness(rect, corner_radius);
    const int segments = 16;
    Rectangle r;
    r.x = rect.x;
    r.y = rect.y;
    r.width = rect.width;
    r.height = rect.height;
    DrawRectangleRoundedLinesEx(r, roundedness, segments, thickness, RCOLOR(color));
}

struct Mui_Font {
    Font raylib_font;
};

Mui_Vector2 mui_measure_text(struct Mui_Font* font, const char *text, float font_size, float spacing, size_t start, size_t end) {
    assert(start <= end);
    char* t_cstr = uti_temp_strndup(&text[start], end - start);
    Vector2 v = MeasureTextEx(font->raylib_font, t_cstr, font_size, spacing);
    return (Mui_Vector2) {
        .x = v.x,
        .y = v.y,
    };
}

// TODO: malloc leaks
struct Mui_Font *mui_load_font_ttf(void* ttf_data, int ttf_data_size, float font_size) {
    struct Mui_Font* font = malloc(sizeof(struct Mui_Font));
    memset(font, 0xCD, sizeof(struct Mui_Font));
    Font f = LoadFontFromMemory(".ttf", ttf_data, ttf_data_size, font_size, 0, 250);
    memcpy(font, &f, sizeof(*font));
    return font;
}

void mui_draw_text_line(struct Mui_Font* font, Mui_Vector2 pos, float letter_space, float letter_size, const char* text, Mui_Color color, size_t start, size_t end) {
    char* t_cstr = uti_temp_strndup(&text[start], end - start);
    DrawTextEx(font->raylib_font, t_cstr, RV2(pos), letter_size, letter_space, RCOLOR(color));
}


