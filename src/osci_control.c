// Copyright (C) 2026 Benjamin Froelich
// This file is part of https://github.com/bbeni/current_pulser_v3
// For conditions of distribution and use, see copyright notice in project root.

#include "stdio.h"
#include "math.h"
#include "stdbool.h"
#include "stdlib.h"
#include "string.h"
#include "assert.h"

#include "dwf.h"

#include "mui.h"
#include "uti.h"

#include "osci_control.h"


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
    plot_args.x_label = "t [s]";
    plot_args.y_label = "U [V]";
    plot_args.thick_y_zero = true;
    plot_args.tick_x_label_fmt = "%.6f";
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

    ui->plot_args.y_top = 1.0 * ui->y_slider_state.value;
    ui->plot_args.y_bot = -1.0 * ui->y_slider_state.value;
    ui->plot_args.x_left = state->t_min_data * ui->t_slider_state.value;
    ui->plot_args.x_right = state->t_max_data * ui->t_slider_state.value;

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
