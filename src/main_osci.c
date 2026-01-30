// Copyright (C) 2026 Benjamin Froelich
// This file is part of https://github.com/bbeni/current_pulser_v3
// For conditions of distribution and use, see copyright notice in project root.
#include "dwf.h"

#include "stdio.h"
#include "math.h"
#include "stdbool.h"
#include "stdlib.h"
#include "string.h"
#include "assert.h"


#include "mui.h"
#include "gra.h"
#include "uti.h"

#ifdef _WIN32
    #include <windows.h>
    #define Wait(ts) Sleep((int)(1000*ts))
#else
    #include <unistd.h>
    #include <sys/time.h>
    #define Wait(ts) usleep((int)(1000000*ts))
#endif


struct Osc_Device {
    HDWF handle;
    double max_freq_hz;
    int n_channels;
    char name[32];
    char serial_number[32];
    STS status;

    // additional info
    bool triggered_measurement_started;
};

void osc_print_last_error() {
    char szError[512];
    FDwfGetLastErrorMsg(szError);
    printf("ERROR: WaveFormSDK error: %s\n", szError);
}

bool osc_open_device(struct Osc_Device* device) {
    // open first device
    // TODO handle if device is already opened by other application. see samples device_enumeration in WaveFormsSDK.
    if (!FDwfDeviceOpen(-1, &device->handle)) return false;
    if (!FDwfAnalogInChannelCount(device->handle, &device->n_channels)) return false;
    if (!FDwfAnalogInFrequencyInfo(device->handle, NULL, &device->max_freq_hz)) return false;
    if (!FDwfEnumDeviceName (0, device->name)) return false;
    if (!FDwfEnumSN(0, device->serial_number)) return false;

    printf("device name:  %s\n", device->name);
    printf("serial nmbr:  %s\n", device->serial_number);
    printf("max frequncy: %.0f\n", device->max_freq_hz);
    printf("number chan:  %d\n", device->n_channels);

    return true;
}


// make sure you free data_out
// UNUSED
void osc_create_measurement(struct Osc_Device* device, double** data_out, int* n_samples_out, float v_pk_to_pk, float sample_rate) {

    // enable channels
    for(int c = 0; c < device->n_channels; c++){
        FDwfAnalogInChannelEnableSet(device->handle, c, true);
    }
    // set 5V pk2pk input range for all channels
    FDwfAnalogInChannelRangeSet(device->handle, -1, v_pk_to_pk);

    // 20MHz sample rate
    FDwfAnalogInFrequencySet(device->handle, sample_rate);

    // get the maximum buffer size
    FDwfAnalogInBufferSizeInfo(device->handle, NULL, n_samples_out);
    FDwfAnalogInBufferSizeSet(device->handle, *n_samples_out);

    double* data;
    data = malloc(sizeof(*data) * *n_samples_out);

    // configure trigger
    FDwfAnalogInTriggerSourceSet(device->handle, trigsrcDetectorAnalogIn);
    FDwfAnalogInTriggerAutoTimeoutSet(device->handle, 10.0);
    FDwfAnalogInTriggerChannelSet(device->handle, 0);
    FDwfAnalogInTriggerTypeSet(device->handle, trigtypeEdge);
    FDwfAnalogInTriggerLevelSet(device->handle, 1.0);
    FDwfAnalogInTriggerConditionSet(device->handle, trigcondRisingPositive);

    // wait at least 2 seconds with Analog Discovery for the offset to stabilize, before the first reading after device open or offset/range change
    Wait(2);

    // start
    FDwfAnalogInConfigure(device->handle, 0, true);

    printf("Waiting for triggered or auto acquisition\n");
    do {
        FDwfAnalogInStatus(device->handle, true, &device->status);
    } while (device->status != stsDone);

    // get the samples for each channel
    for (int c = 0; c < device->n_channels; c++) {
        FDwfAnalogInStatusData(device->handle, c, data, *n_samples_out);
    }

    *data_out = data;
}

// remember to free data_out
void osc_shift_screen_setup(struct Osc_Device* device, double** data_out, int* n_samples_out, float v_pk_to_pk, float sample_rate) {

    // enable channels
    for(int c = 0; c < device->n_channels; c++){
        FDwfAnalogInChannelEnableSet(device->handle, c, true);
    }

    FDwfAnalogInChannelRangeSet(device->handle, -1, v_pk_to_pk);
    FDwfAnalogInFrequencySet(device->handle, sample_rate);

    // get the maximum buffer size
    FDwfAnalogInBufferSizeInfo(device->handle, NULL, n_samples_out);
    FDwfAnalogInBufferSizeSet(device->handle, *n_samples_out);
    FDwfAnalogInAcquisitionModeSet(device->handle, acqmodeScanScreen);

    double* data;
    data = malloc(sizeof(*data) * *n_samples_out);
    memset(data, 0x0, sizeof(*data) * *n_samples_out);

    *data_out = data;

    // start
    FDwfAnalogInConfigure(device->handle, 0, true);
}

void osc_shift_screen_update(struct Osc_Device* device, double* data_out, int n_samples) {

    if (!FDwfAnalogInStatus(device->handle, true, &device->status)) return;
    if (device->status == stsDone) return;

    // get the samples for each channel
    for (int c = 0; c < device->n_channels; c++) {
        FDwfAnalogInStatusData(device->handle, c, data_out, n_samples);
    }
}


typedef enum {
    OSC_TRIGGER_TYPE_EDGE = 0, // trigtypeEdge,
    OSC_TRIGGER_TYPE_PULSE = 1, // trigtypePulse,
    OSC_TRIGGER_TYPE_TRANSITION = 2, // trigtypeTransition,
    OSC_TRIGGER_TYPE_WINDOW = 3, // trigtypeWindow,
} OSC_TRIGGER_TYPE;


typedef enum {
    OSC_TRIGGER_CONDITION_RISING_POSITIVE  = 0, //trigcondRisingPositive   = 0;
    OSC_TRIGGER_CONDITION_FALLING_NEGATIVE = 1, //trigcondFallingNegative  = 1;
} OSC_TRIGGER_CONDITION;


// remember to free data_out
bool osc_triggered_setup(struct Osc_Device* device, double** data_out, int* n_samples_out, float v_pk_to_pk, float sample_rate) {

    // enable channels
    for(int c = 0; c < device->n_channels; c++){
        if (!FDwfAnalogInChannelEnableSet(device->handle, c, true)) return false;
    }

    // all channels
    if (!FDwfAnalogInChannelRangeSet(device->handle, -1, v_pk_to_pk)) return false;
    if (!FDwfAnalogInFrequencySet(device->handle, sample_rate)) return false;

    // get the maximum buffer size
    if (!FDwfAnalogInBufferSizeInfo(device->handle, NULL, n_samples_out)) return false;
    if (!FDwfAnalogInBufferSizeSet(device->handle, *n_samples_out)) return false;
    if (!FDwfAnalogInAcquisitionModeSet(device->handle, acqmodeSingle)) return false;

    double* data;
    data = malloc(sizeof(*data) * *n_samples_out);
    memset(data, 0x0, sizeof(*data) * *n_samples_out);

    *data_out = data;

    // start
    if (!FDwfAnalogInConfigure(device->handle, 0, true)) return false;
    return false;
}

// wait at least 2 seconds with Analog Discovery for the offset to stabilize, before the first reading after device open or offset/range change
bool osc_triggered_arm_trigger(struct Osc_Device* device, float time_out, int channel, float level, float position, OSC_TRIGGER_TYPE type, OSC_TRIGGER_CONDITION condition) {
    // configure trigger
    if (!FDwfAnalogInTriggerSourceSet(device->handle, trigsrcDetectorAnalogIn)) return false;
    if (!FDwfAnalogInTriggerAutoTimeoutSet(device->handle, time_out)) return false;
    if (!FDwfAnalogInTriggerChannelSet(device->handle, channel)) return false;
    if (!FDwfAnalogInTriggerTypeSet(device->handle, type)) return false;
    if (!FDwfAnalogInTriggerLevelSet(device->handle, level)) return false;
    if (!FDwfAnalogInTriggerConditionSet(device->handle, condition)) return false;
    if (!FDwfAnalogInTriggerPositionSet(device->handle, position)) return false;
    device->triggered_measurement_started = false;
    return true;
}

// cooldown after configuring the trigger needs to be at least 2s
// returns true when done
bool osc_triggered_update(struct Osc_Device* device, float trigger_cooldown, double* data_out, int n_samples) {

    if (trigger_cooldown > 0) return false;

    if (!device->triggered_measurement_started) {
        // start (waiting for trigger)
        FDwfAnalogInConfigure(device->handle, 0, true);
        device->triggered_measurement_started = true;
    }

    // read data if it is here
    FDwfAnalogInStatus(device->handle, true, &device->status);
    if (device->status != stsDone) return false;

    // get the samples for each channel
    for (int c = 0; c < device->n_channels; c++) {
        FDwfAnalogInStatusData(device->handle, c, data_out, n_samples);
    }

    return true;
}

void osc_cleanup_data(double* data) {
    free(data);
}


struct Oscilloscope_State {
    struct Osc_Device device;
    bool device_available;

    // TODO: multi channel support
    double* data;
    double* t_data;
    int n_data;

    double* display_data;
    double* display_t_data;
    int n_display_data;

    // derived
    double step;
    double t_total;
    double t_min_data;
    double t_max_data;
};

struct Oscilloscope_Ui {
    Mui_Checkbox_State trigger_armed_cb_state;
    Mui_Slider_State y_slider_state;
    Mui_Slider_State t_slider_state;

    float trigger_armed_timestamp;
    bool triggerd_data_aquired;

    float TRIGGER_ARM_COOLDOWN;

    struct Gra_Gridded_Base_Arguments plot_args;
};

struct Oscilloscope_Settings {
    float sample_rate;
    float v_pk_to_pk;

    bool trigger_mode; // either that or shift window
    int trigger_channel;
    float trigger_level; // volts
    float trigger_position; // in seconds
    float trigger_timeout; // in seconds
    OSC_TRIGGER_TYPE trigger_type;
    OSC_TRIGGER_CONDITION trigger_condition;
};



bool oscilloscope_setup(struct Oscilloscope_State* state, struct Oscilloscope_Settings* settings) {

    if (!osc_open_device(&state->device)) {
        osc_print_last_error();
        state->device_available = false;
        return false;
    }
    state->device_available = true;

    double* data;
    int n_data;
    if (settings->trigger_mode) {
        if (!osc_triggered_setup(&state->device, &data, &n_data, settings->v_pk_to_pk, settings->trigger_level)) {
            osc_print_last_error();
            state->device_available = false;
            return false;
        }
    } else {
        osc_shift_screen_setup(&state->device, &data, &n_data, settings->v_pk_to_pk, settings->sample_rate);
    }
    state->data = data;
    state->n_data = n_data;

    printf("setup %d data_point measurement\n", n_data);

    //
    // setup arrays for data interpolation
    //
    state->step = 1.0 / settings->sample_rate;
    state->t_total = (n_data - 1) * state->step;
    state->t_min_data = -state->t_total  * 0.5f;
    state->t_max_data = state->t_min_data + state->t_total;

    size_t n_interpol = 3000;

    state->t_data = malloc(sizeof(*state->t_data) * n_data);
    state->display_t_data = malloc(sizeof(*state->display_t_data) * n_interpol);
    state->display_data = malloc(sizeof(*state->display_t_data) * n_interpol);

    for (int i = 0; i < n_data; i++) {
        state->t_data[i] = state->t_min_data + i * state->step;
    }

    return true;
}

struct Oscilloscope_Ui_Settings {
    float x;
};

void oscilloscope_ui_setup(struct Oscilloscope_Ui* oscilloscope_ui, struct Oscilloscope_Ui_Settings* ui_settings, float grid_pixels_unit) {
    oscilloscope_ui->TRIGGER_ARM_COOLDOWN = 2.0f;
    oscilloscope_ui->trigger_armed_timestamp = -10000.0f;
    oscilloscope_ui->triggerd_data_aquired = false;
    oscilloscope_ui->y_slider_state.value = 1.0f;
    oscilloscope_ui->t_slider_state.value = 1.0f;

    struct Gra_Gridded_Base_Arguments plot_args;
    plot_args.grid_unit_pixels = grid_pixels_unit;
    plot_args.grid_w = 14;
    plot_args.grid_h = 10;
    plot_args.grid_left_axis_off = 2;
    plot_args.grid_bot_axis_off = 2;
    plot_args.grid_skip_x = 1;
    plot_args.grid_skip_y = 1;
    plot_args.x_left = -0.1f;
    plot_args.x_right = 0.1f;
    plot_args.y_bot = 2.5f;
    plot_args.y_top = -2.5f;
    plot_args.x_label = "t [ms]";
    plot_args.y_label = "U [V]";
    plot_args.thick_y_zero = true;
    plot_args.tick_x_label_fmt = "%.2f";
    plot_args.tick_y_label_fmt = "%.2f";

    oscilloscope_ui->plot_args = plot_args;
}

void oscilloscope_change_mode(struct Oscilloscope_State* state, struct Oscilloscope_Ui* ui, struct Oscilloscope_Settings* settings, bool triggered) {

    if (settings->trigger_mode == triggered) return;

    if (triggered) {
        // TRIGGERED DATA setup
        osc_cleanup_data(state->data);
        osc_triggered_setup(&state->device, &state->data, &state->n_data, settings->v_pk_to_pk, settings->sample_rate);
        osc_triggered_arm_trigger(&state->device, 1e23, 0, 0.05f, 0.0008f, OSC_TRIGGER_TYPE_EDGE, OSC_TRIGGER_CONDITION_RISING_POSITIVE);
        ui->trigger_armed_timestamp = mui_get_time();
        ui->triggerd_data_aquired = false;
    } else {
        // SHIFT_SCREEN DATA setup
        osc_cleanup_data(state->data);
        osc_shift_screen_setup(&state->device, &state->data, &state->n_data, settings->v_pk_to_pk, settings->sample_rate);
        ui->trigger_armed_timestamp = mui_get_time() - ui->TRIGGER_ARM_COOLDOWN;
    }
}

void oscilloscope_ui_draw(Mui_Rectangle area, float grid_pixel_unit, struct Oscilloscope_Ui* ui, struct Oscilloscope_State* state, struct Oscilloscope_Settings* settings) {

    Mui_Rectangle trigger_menu_bar_rect;
    Mui_Rectangle trigger_menu_rect;
    Mui_Rectangle scope_rect = mui_cut_top(area, 0.66666f * grid_pixel_unit, &trigger_menu_bar_rect);
    scope_rect = mui_cut_top(scope_rect, 0.33333f * grid_pixel_unit, NULL);

    mui_cut_right(trigger_menu_bar_rect, 5 * grid_pixel_unit, &trigger_menu_rect);

    float trigger_armed_cooldown = ui->TRIGGER_ARM_COOLDOWN + ui->trigger_armed_timestamp - mui_get_time();

    char trigger_label_text[40];
    if (trigger_armed_cooldown > 0) {
        snprintf(trigger_label_text, 39, "TRIGGER (%.1f s)", trigger_armed_cooldown);
    } else {
        if (ui->trigger_armed_cb_state.checked) {
            if (ui->triggerd_data_aquired)
                snprintf(trigger_label_text, 39, "TRIGGER (aquired)");
            else
                snprintf(trigger_label_text, 39, "TRIGGER (armed)");
        } else {
            snprintf(trigger_label_text, 39, "TRIGGER");
        }
    }

    if (state->device_available) {
        if (mui_checkbox(&ui->trigger_armed_cb_state, trigger_label_text, trigger_menu_rect)) {
            // checkbox toggeled
            bool triggered = ui->trigger_armed_cb_state.checked;
            oscilloscope_change_mode(state, ui, settings, triggered);
        }
    }

    Mui_Rectangle y_slider_rect;
    scope_rect = mui_cut_right(scope_rect, 1 * grid_pixel_unit, &y_slider_rect);
    Mui_Rectangle t_slider_rect;
    scope_rect = mui_cut_top(scope_rect, 1 * grid_pixel_unit, &t_slider_rect);
    y_slider_rect = mui_cut_top(y_slider_rect, 1 * grid_pixel_unit, NULL);

    mui_simple_slider(&ui->y_slider_state, true, y_slider_rect);
    mui_simple_slider(&ui->t_slider_state, false, t_slider_rect);

    double t_min = state->t_min_data * ui->t_slider_state.value;
    double t_max = state->t_max_data * ui->t_slider_state.value;
    double y_min = -1.0 * ui->y_slider_state.value;
    double y_max = 1.0 * ui->y_slider_state.value;
    double t_step = 0.00001; // 10 us
    double y_step = 0.1;   // 0,1 V

    //Mui_Rectangle plot_rect = gra_xy_plot_labels_and_grid("t [s]", "A [V]", t_min, t_max, y_min, y_max, t_step, y_step, true, scope_rect);
    Mui_Rectangle plot_rect = gra_gridded_xy_base(&ui->plot_args, scope_rect);

    if (!state->device_available) return;

    for (int i = 0; i < state->n_display_data; i++) {
        state->display_t_data[i] = t_min + (t_max - t_min) * i / (state->n_display_data - 1);
    }
    // TODO: mui: rename x_resamples to ..._out for consistency
    mma_spline_cubic_natural(state->display_t_data, state->data, state->n_data, state->display_data, state->display_t_data, state->n_display_data);
    gra_xy_plot_data_points(state->display_t_data, state->display_data, NULL, state->n_display_data, t_min, t_max, y_min, y_max, MUI_YELLOW, 2.0f, plot_rect);
    if ( (state->t_max_data - state->t_min_data) / (t_max - t_min) * state->n_display_data / state->n_data > 5 ) {
        gra_xy_plot_data_points(state->t_data, state->data, NULL, state->n_data, t_min, t_max, y_min, y_max, MUI_RED, 2.0f, plot_rect);
    }

}


void oscilloscope_ui_update(struct Oscilloscope_Ui* oscilloscope_ui, struct Oscilloscope_State* oscilloscope_state) {
    float trigger_armed_cooldown = oscilloscope_ui->TRIGGER_ARM_COOLDOWN + oscilloscope_ui->trigger_armed_timestamp - mui_get_time();

    if (oscilloscope_ui->trigger_armed_cb_state.checked) {
        if (!oscilloscope_ui->triggerd_data_aquired) {
            if (osc_triggered_update(&oscilloscope_state->device, trigger_armed_cooldown, oscilloscope_state->data, oscilloscope_state->n_data)) {
                oscilloscope_ui->triggerd_data_aquired = true;
            }
        }
    } else {
        osc_shift_screen_update(&oscilloscope_state->device, oscilloscope_state->data, oscilloscope_state->n_data);
    };
}

void oscilloscope_destroy(struct Oscilloscope_State* oscilloscope_state) {
    if (oscilloscope_state->device_available)
        FDwfDeviceClose(oscilloscope_state->device.handle);
}


int main() {

    int grid_w = 18;
    int grid_h = 15;
    int grid_pixel_unit = 50;

    int w, h;
    w = grid_w * grid_pixel_unit;
    h = grid_h * grid_pixel_unit;

    mui_open_window(w, h, 500, 200, "Current Pulser V3 Controller", 1.0f, MUI_WINDOW_RESIZEABLE /* | MUI_WINDOW_UNDECORATED */, NULL);
    mui_init_themes(0, 0, false, "resources/font/NimbusSans-Regular.ttf");

    struct Oscilloscope_Settings settings;
    settings.sample_rate  = 7692300.0f;  // Hz
    settings.v_pk_to_pk = 5.0f;          // volts
    settings.trigger_mode = false;
    settings.trigger_channel = 0;
    settings.trigger_level = 0.05f;      // volts
    settings.trigger_position = 0.0008;  // in seconds
    settings.trigger_timeout = 1e23;     // in seconds
    settings.trigger_type = OSC_TRIGGER_TYPE_EDGE;
    settings.trigger_condition = OSC_TRIGGER_CONDITION_RISING_POSITIVE;
    struct Oscilloscope_State oscilloscope_state = {0};
    oscilloscope_setup(&oscilloscope_state, &settings);

    struct Oscilloscope_Ui_Settings ui_settings;
    struct Oscilloscope_Ui oscilloscope_ui_state = {0};
    oscilloscope_ui_setup(&oscilloscope_ui_state, &ui_settings, grid_pixel_unit);

    while (!mui_window_should_close())
    {
        mui_update_core();

        oscilloscope_ui_update(&oscilloscope_ui_state, &oscilloscope_state);


        w = mui_screen_width();
        h = mui_screen_height();

        mui_begin_drawing();
        mui_clear_background(mui_protos_theme_g.bg_light, NULL);

        Mui_Rectangle whole_screen = mui_rectangle(0, 0, w, h);

        const float decoration_height = grid_pixel_unit;
        //Mui_Rectangle menu_bar_area = mui_window_decoration(decoration_height, true, true, true, true, true, whole_screen);
        Mui_Rectangle screen = mui_cut_top(whole_screen, decoration_height, NULL);
        //mui_label(&mui_protos_theme_g, "PULSER V3 OSCILLOSCOPE", MUI_TEXT_ALIGN_DEFAULT, menu_bar_area);

        Mui_Rectangle scope_rect = mui_shrink(screen, grid_pixel_unit);

        oscilloscope_ui_draw(scope_rect, grid_pixel_unit, &oscilloscope_ui_state, &oscilloscope_state, &settings);


        mui_end_drawing();
        uti_temp_reset();
    }

    oscilloscope_destroy(&oscilloscope_state);
    mui_close_window();

    return 0;
}