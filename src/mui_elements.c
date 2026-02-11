#include "mui.h"
#include "uti.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

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

Mui_Text_Selectable_State mui_text_selectable_state() {
    Mui_Text_Selectable_State state;
    state.mouse_down_pivot_cursor = 0;
    state.mouse_down_selectable_text = false;
    state.selector_1 = 0;
    state.selector_2 = 0;
    return state;
}

Mui_Number_Input_State mui_number_input_state(double inital_value) {
    Mui_Number_Input_State state;
    state.text_selectable_state = mui_text_selectable_state();
    state.active = false;
    state.text[sizeof(state.text) - 1] = '\0';
    state.text_length = 0;
    state.parsed_number = inital_value;
    state.parsed_valid = true;
    state.parsed = true;
    state.theme = &mui_protos_theme_g;
    return state;
}


Mui_Vector2 _internal_get_text_draw_position_by_align(MUI_TEXT_ALIGN_FLAGS text_align_flags, Mui_Vector2 text_measure, Mui_Rectangle place) {

    Mui_Vector2 position;
    position.x = place.x;
    position.y = place.y;

    if (text_align_flags == MUI_TEXT_ALIGN_DEFAULT) {
        text_align_flags = MUI_TEXT_ALIGN_LEFT | MUI_TEXT_ALIGN_MID;
    }

    if (text_align_flags & MUI_TEXT_ALIGN_LEFT) position.x += text_measure.y * 0.25f;
    else if (text_align_flags & MUI_TEXT_ALIGN_CENTER) position.x += (place.width - text_measure.x) * 0.5f;
    else if (text_align_flags & MUI_TEXT_ALIGN_RIGHT) position.x += (place.width - text_measure.x) - text_measure.y * 0.25f;

    if (text_align_flags & MUI_TEXT_ALIGN_TOP) {}
    else if (text_align_flags & MUI_TEXT_ALIGN_MID) position.y += (place.height - text_measure.y) * 0.5f;
    else if (text_align_flags & MUI_TEXT_ALIGN_BOTTOM) position.y += (place.height - text_measure.y);
    else position.y += (place.height - text_measure.y) * 0.5f; // defualt mid

    return position;
}

void mui_label(Mui_Theme *theme, char *text, MUI_TEXT_ALIGN_FLAGS text_align_flags, Mui_Rectangle place) {
    if (theme == NULL) {
        theme = &mui_protos_theme_g;
    }

    //mui_draw_rectangle_rounded(place, theme->corner_radius, theme->bg);

    int l = mui_text_len(text, strlen(text));
    Mui_Vector2 text_measure = mui_measure_text(theme->font, text, theme->label_text_size, 0.0f, 0, l);
    Mui_Vector2 position = _internal_get_text_draw_position_by_align(text_align_flags, text_measure, place);
    mui_draw_text_line(theme->label_font, position, 0.0f, theme->label_text_size, text, theme->text, 0, l);
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

// returns true when clicked
bool mui_checkbox(Mui_Checkbox_State *state, const char *text, Mui_Rectangle place) {

    bool clicked = false;

    Mui_Theme *theme = state->theme;
    if (theme == NULL) theme = &mui_protos_theme_g;

    // Update the time
    float dt = mui_get_time() - state->last_time;
    state->last_time = mui_get_time();

    if (mui_is_inside_rectangle(mui_get_mouse_position(), place)) {
        if (mui_is_mouse_button_pressed(0)) {
            state->checked = !state->checked;
            clicked = true;
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

    return clicked;
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
        if (vertical) s = 1.0f - fmin(fmax(0.0f, (mpos.y - place.y) / place.height), 1.0f);
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
        wagon.y = (1.0f - state->value) * w + place.y;
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

    size_t l = mui_text_len(text, strlen(text));
    Mui_Vector2 text_meaurement = mui_measure_text(theme->label_font, text, theme->font_size, 0.1f, 0, l);
    Mui_Vector2 position;
    position.x = place.x +  (place.width - text_meaurement.x) * 0.5f;
    position.y = place.y + place.height / 2 - theme->font_size / 2;
    mui_draw_text_line(theme->label_font, position, 0, theme->font_size, text, text_color, 0, l);
    return returnstate;
}


bool mui_n_status_button(Mui_Button_State *state, const char* text, const Mui_Color* status_colors_array, int status_count, int status, Mui_Rectangle place) {

    assert(status < status_count);
    assert(status >= 0);

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

    bg = mui_interpolate_color(theme->bg, theme->primary_dark, state->hover_t);
    bg = status_colors_array[status];

    float outline_thickness = 2.0f;

    mui_draw_rectangle_rounded(place, theme->corner_radius - 0.25f * theme->corner_radius, bg);
    mui_draw_rectangle_rounded_lines(mui_shrink(place, outline_thickness), theme->corner_radius, theme->border, outline_thickness);


    Mui_Color text_color = mui_interpolate_color(theme->text, theme->primary, state->hover_t);

    size_t l = mui_text_len(text, strlen(text));
    Mui_Vector2 text_meaurement = mui_measure_text(theme->label_font, text, theme->font_size, 0.1f, 0, l);
    Mui_Vector2 position;
    position.x = place.x +  (place.width - text_meaurement.x) * 0.5f;
    position.y = place.y + place.height / 2 - theme->font_size / 2;
    mui_draw_text_line(theme->label_font, position, 0, theme->font_size, text, text_color, 0, l);
    return returnstate;

}


void mui_n_status_label(Mui_Theme* theme, const char* text, const Mui_Color* status_colors_array, int status_count, int status, MUI_TEXT_ALIGN_FLAGS text_align_flags, Mui_Rectangle place) {

    assert(status < status_count);
    assert(status >= 0);

    Mui_Color text_color = theme->text;
    Mui_Color bg = status_colors_array[status];
    float outline_thickness = 2.0f;

    mui_draw_rectangle_rounded(place, theme->corner_radius - 0.25f * theme->corner_radius, bg);
    mui_draw_rectangle_rounded_lines(mui_shrink(place, outline_thickness), theme->corner_radius, theme->border, outline_thickness);

    size_t l = mui_text_len(text, strlen(text));
    Mui_Vector2 text_measure = mui_measure_text(theme->font, text, theme->label_text_size, 0.1f, 0, l);
    Mui_Vector2 position = _internal_get_text_draw_position_by_align(text_align_flags, text_measure, place);

    mui_draw_text_line(theme->label_font, position, 0, theme->font_size, text, text_color, 0, l);

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
size_t _internal_get_cursor_by_position(Mui_Vector2 pos, char* text, size_t* start_cursor, size_t* end_cursor, size_t n_lines, struct Mui_Font* font, float font_size, Mui_Rectangle place);


// return true if the value changed
bool mui_number_input(Mui_Number_Input_State *state, Mui_Rectangle place) {

    Mui_Theme *theme = state->theme;
    if (theme == NULL) {
        theme = &mui_protos_theme_g;
    }

    float font_size = theme->textinput_text_size;
    struct Mui_Font *font = theme->textinput_font;

    size_t selector_1 = state->text_selectable_state.selector_1;
    size_t selector_2 = state->text_selectable_state.selector_2;

    if (mui_is_mouse_button_pressed(0)) {
        if (mui_is_inside_rectangle(mui_get_mouse_position(), place)) {
            state->active = true;
        } else {
            state->active = false;
            selector_1 = 0;
            selector_2 = 0;
        }
    }

    if (state->active) {

        if (mui_is_key_pressed(MUI_KEY_ENTER) || mui_is_key_pressed_repeat(MUI_KEY_ENTER)) {
            bool success = uti_parse_number(state->text, MUI_NUMBER_INPUT_MAX_INPUT_SIZE, &state->parsed_number);
            state->parsed = true;
            state->parsed_valid = success;
            if (success) {
                selector_2 = state->text_length;
                selector_1 = state->text_length;
            }
        }

        // handle text input
        int unicode_char = mui_get_char_pressed();
        if (unicode_char != 0) {
            unsigned char c = unicode_char & 0xff; // TODO make it better

            bool is_char_writable = isdigit(c) || c == '.' || c == 'e' || c == '-';

            if (is_char_writable) {
                if (selector_1 == selector_2) {
                    // normal cursor mode
                    if (state->text_length <= MUI_NUMBER_INPUT_MAX_INPUT_SIZE - 1) {
                        for (size_t i = state->text_length; i > selector_1; i--) {
                            state->text[i] = state->text[i - 1];
                        }
                        state->text[selector_1] = c;
                        state->text_length++;
                        selector_1++;
                        selector_2++;
                        state->text[state->text_length] = '\0';
                    }
                } else {
                    // selected text
                    assert(selector_1 < selector_2);
                    size_t diff = selector_2 - selector_1 - 1;
                    for (size_t i = selector_1 + 1; i <= state->text_length && i + diff <= state->text_length; i++) {
                        state->text[i] = state->text[i + diff];
                    }
                    state->text[selector_1] = c;
                    state->text_length -= diff;
                    selector_2 -= diff;
                    selector_1 = selector_2;
                }
            }
        }

        // handle deleting stuff
        if (mui_is_key_pressed(MUI_KEY_BACKSPACE) || mui_is_key_pressed_repeat(MUI_KEY_BACKSPACE)) {
            if (selector_1 == selector_2) {
                // normal cursor mode
                if (selector_1 > 0) {
                    // xxx|x
                    for (size_t i = selector_1 - 1; i < state->text_length; i++) {
                        state->text[i] = state->text[i + 1];
                    }
                    state->text[state->text_length] = '\0';
                    selector_1--;
                    selector_2--;
                    state->text_length--;
                }
            } else {
                // selected text
                assert(selector_1 < selector_2);
                size_t diff = selector_2 - selector_1;
                for (size_t i = selector_1; i <= state->text_length && i + diff <= state->text_length; i++) {
                    state->text[i] = state->text[i + diff];
                }
                state->text_length -= diff;
                selector_2 -= diff;
            }
        }


        // handle cursor and selection movement with keyboard
        if (mui_is_key_pressed(MUI_KEY_LEFT) || mui_is_key_pressed_repeat(MUI_KEY_LEFT)) {
            if (selector_1 == selector_2) {
                if (selector_1 != 0) selector_1--;
                if (selector_2 != 0) selector_2--;
            }

            if (selector_1 != selector_2) {
                if (mui_is_key_down(MUI_KEY_LEFT_SHIFT) || mui_is_key_down(MUI_KEY_RIGHT_SHIFT)) {
                    if (selector_1 != 0) selector_1--;
                } else {
                    selector_2 = selector_1;
                }
            }
        }

        if (mui_is_key_pressed(MUI_KEY_RIGHT) || mui_is_key_pressed_repeat(MUI_KEY_RIGHT)) {
            if (selector_1 == selector_2) {
                if (selector_1 < state->text_length) selector_1++;
                if (selector_2 < state->text_length) selector_2++;
            }

            if (selector_1 != selector_2) {
                if (mui_is_key_down(MUI_KEY_LEFT_SHIFT) || mui_is_key_down(MUI_KEY_RIGHT_SHIFT)) {
                    if (selector_2 < state->text_length) selector_2++;
                } else {
                    selector_1 = selector_2;
                }
            }
        }
    }

    state->text_selectable_state.selector_1 = selector_1;
    state->text_selectable_state.selector_2 = selector_2;

    // draw text
    const float padding = 5.0f;
    place = mui_shrink(place, padding);

    bool number_changed = false;
    if (state->parsed_valid && state->parsed) {
        uti_render_postfix_number(state->text, MUI_NUMBER_INPUT_MAX_INPUT_SIZE, state->parsed_number);
        state->text_length = strlen(state->text);
        mui_text_selectable(&state->text_selectable_state, state->text, place);
        state->parsed = false;
        number_changed = true;
    } else {
        mui_text_selectable(&state->text_selectable_state, state->text, place);
    }

    // cursor draw
    if (state->active && selector_1 == selector_2) {
        Mui_Rectangle rect_cursor;
        rect_cursor.x = place.x + ceil(font_size / 6);
        rect_cursor.y = place.y + padding;
        rect_cursor.height = font_size;
        rect_cursor.width = font_size / 11;

        Mui_Vector2 measure = mui_measure_text(font, state->text, font_size, 0.1f, 0, selector_1);
        rect_cursor.x += measure.x;
        mui_draw_rectangle(rect_cursor, MUI_BLACK);
    }

    // return true if the number changed
    return number_changed;
}



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
// TODO: make this part of a state
void mui_text_selectable(Mui_Text_Selectable_State* state, char* text, Mui_Rectangle place) {
    Mui_Theme *theme = &mui_protos_theme_g;
    float font_size = theme->textinput_text_size;
    Mui_Color text_color = theme->text;
    struct Mui_Font* font = theme->textinput_font;

    size_t total_length = strlen(text);
    mui_draw_rectangle(place, theme->bg_light);
    place = mui_shrink(place, ceil(font_size / 6));

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
    if ((size_t)(prev - text) < total_length) {
        start_cursor[line_nr] = prev - text;
        end_cursor[line_nr] = total_length;
        line_nr++;
    }

    n_lines = line_nr;

    //
    // input handling
    //
    // TODO: fix mouse cursor ibeam disapearing with multiple active text fields.
    if (mui_is_inside_rectangle(mui_get_mouse_position(), place)) {
        mui_set_mouse_cursor(MUI_MOUSE_CURSOR_IBEAM);
    } else {
        mui_set_mouse_cursor(MUI_MOUSE_CURSOR_DEFAULT);
    }
    if (mui_is_mouse_button_pressed(0) && mui_is_inside_rectangle(mui_get_mouse_position(), place)) {
        state->mouse_down_selectable_text = true;
        Mui_Vector2 mouse = mui_get_mouse_position();
        size_t new_cursor = _internal_get_cursor_by_position(mouse, text, start_cursor, end_cursor, n_lines, font, font_size, place);
        state->selector_1 = new_cursor;
        state->selector_2 = new_cursor;
        state->mouse_down_pivot_cursor = new_cursor;
    }
    // TODO: also handle other events that should cancel this thing, like mouse goes out of window.
    if (mui_is_mouse_button_up(0)) state->mouse_down_selectable_text = false;

    // TODO: implement copy paste
    /*
    if (mui_is_key_down(MUI_KEY_LEFT_CONTROL) || mui_is_key_down(MUI_KEY_RIGHT_CONTROL)) {
        if (mui_is_key_pressed(MUI_KEY_C)) {
            if (stage_view->selector_start < stage_view->selector_end) {
                char* temp_str = uti_temp_strndup(&stage_view->selectable_text[stage_view->selector_start], stage_view->selector_end - stage_view->selector_start);
                mui_set_clipboard_text(temp_str);
            }
        }
    }

    */

    if (state->mouse_down_selectable_text) {
        Mui_Vector2 mouse = mui_get_mouse_position();
        size_t new_cursor = _internal_get_cursor_by_position(mouse, text, start_cursor, end_cursor, n_lines, font, font_size, place);
        state->selector_1 = state->mouse_down_pivot_cursor;
        state->selector_2 = state->mouse_down_pivot_cursor;
        if (new_cursor > state->mouse_down_pivot_cursor) {
            // to the right
            state->selector_2 = new_cursor;
        }
        if (new_cursor < state->mouse_down_pivot_cursor) {
            // to the left
            state->selector_1 = new_cursor;
        }
    }

    // clamp selectors
    state->selector_1 = max(state->selector_1, 0);
    state->selector_2 = min(state->selector_2, total_length);

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

        if(state->selector_1 >= line_start && state->selector_1 <= line_end) {
            selection_start_line = i;
            selection_start_line_offset = state->selector_1 - line_start;
            Mui_Vector2 s1 = mui_measure_text(font, text, font_size, 0.1f, line_start, line_start + selection_start_line_offset);
            selection_start_x = s1.x;
        }

        if(state->selector_2 >= line_start && state->selector_2 <= line_end) {
            selection_end_line = i;
            selection_end_line_offset = state->selector_2 - line_start;
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
