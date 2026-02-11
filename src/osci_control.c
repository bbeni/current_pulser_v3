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


// request_n_samples can be a number or -1 to get the maximum possible. remember to free data_out.
bool osc_shift_screen_setup(struct Osc_Device* device, double** data_out, int request_n_samples, int* n_samples_out, float v_pk_to_pk, float sample_rate) {

    // enable channels
    for(int c = 0; c < device->n_channels; c++){
        if (!FDwfAnalogInChannelEnableSet(device->handle, c, true)) return false;
    }

    if (!FDwfAnalogInChannelRangeSet(device->handle, -1, v_pk_to_pk)) return false;
    if (!FDwfAnalogInFrequencySet(device->handle, sample_rate)) return false;

    // get the maximum buffer size
    if (request_n_samples = -1) {
        // get the maximum buffer size
        if (!FDwfAnalogInBufferSizeInfo(device->handle, NULL, n_samples_out)) return false;
    } else {
        *n_samples_out = request_n_samples;
    }

    if (!FDwfAnalogInBufferSizeSet(device->handle, *n_samples_out)) return false;
    if (!FDwfAnalogInAcquisitionModeSet(device->handle, acqmodeScanScreen)) return false;

    double* data;
    data = malloc(sizeof(*data) * *n_samples_out);
    memset(data, 0x0, sizeof(*data) * *n_samples_out);

    *data_out = data;

    // start
    if (!FDwfAnalogInConfigure(device->handle, 0, true)) return false;
    return true;
}

bool osc_shift_screen_update(struct Osc_Device* device, double* data_out, int n_samples) {

    if (!FDwfAnalogInStatus(device->handle, true, &device->status)) return false;
    if (device->status == stsDone) return false;

    // get the samples for each channel
    //for (int c = 0; c < device->n_channels; c++) {
        FDwfAnalogInStatusData(device->handle, 0, data_out, n_samples);
    //}

    return true;
}


// request_n_samples can be a number or -1 to get the maximum possible. remember to free data_out.
bool osc_triggered_setup(struct Osc_Device* device, double** data_out, int request_n_samples, int* n_samples_out, float v_pk_to_pk, float sample_rate) {

    // enable channels
    for(int c = 0; c < device->n_channels; c++){
        if (!FDwfAnalogInChannelEnableSet(device->handle, c, true)) return false;
    }

    // all channels
    if (!FDwfAnalogInChannelRangeSet(device->handle, -1, v_pk_to_pk)) return false;
    if (!FDwfAnalogInFrequencySet(device->handle, sample_rate)) return false;

    if (request_n_samples = -1) {
        // get the maximum buffer size
        if (!FDwfAnalogInBufferSizeInfo(device->handle, NULL, n_samples_out)) return false;
    } else {
        *n_samples_out = request_n_samples;
    }

    if (!FDwfAnalogInBufferSizeSet(device->handle, *n_samples_out)) return false;
    if (!FDwfAnalogInAcquisitionModeSet(device->handle, acqmodeSingle)) return false;

    double* data;
    data = malloc(sizeof(*data) * *n_samples_out);
    memset(data, 0x0, sizeof(*data) * *n_samples_out);

    *data_out = data;

    // start
    if (!FDwfAnalogInConfigure(device->handle, 0, true)) return false;
    return true;
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
    //for (int c = 0; c < device->n_channels; c++) {
        FDwfAnalogInStatusData(device->handle, 0, data_out, n_samples);
    //}

    return true;
}

void osc_cleanup_data(double* data) {
    free(data);
}

bool oscilloscope_setup(struct Oscilloscope_State* state, const struct Oscilloscope_Settings* settings) {

    if (!osc_open_device(&state->device)) {
        osc_print_last_error();
        state->device_available = false;
        return false;
    }
    state->device_available = true;

    double* data;
    int n_data;
    if (settings->trigger_mode) {
        if (!osc_triggered_setup(&state->device, &data, settings->request_n_samples, &n_data, settings->v_pk_to_pk, settings->trigger_level)) {
            osc_print_last_error();
            state->device_available = false;
            return false;
        }
    } else {
        if (!osc_shift_screen_setup(&state->device, &data, settings->request_n_samples, &n_data, settings->v_pk_to_pk, settings->sample_rate)) {
            osc_print_last_error();
            state->device_available = false;
            return false;
        }
    }
    state->data = data;
    state->n_data = n_data;

    printf("setup %d data_point measurement\n", n_data);

    //
    // setup time data and arrays for data interpolation
    //
    state->step = 1.0 / settings->sample_rate;
    printf("step %.10f\n", state->step);
    state->t_total = (n_data - 1) * state->step;
    state->t_min_data = -(state->t_total * 0.5f) - settings->trigger_position;
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

void oscilloscope_ui_setup(struct Oscilloscope_Ui* oscilloscope_ui, const struct Oscilloscope_Ui_Settings* ui_settings, float grid_pixels_unit) {
    oscilloscope_ui->TRIGGER_ARM_COOLDOWN = 2.0f;
    oscilloscope_ui->trigger_armed_timestamp = -10000.0f;
    oscilloscope_ui->triggerd_data_aquired = false;

    oscilloscope_ui->current_voltage_factor_chan_a = ui_settings->current_voltage_factor_chan_a;
    oscilloscope_ui->current_voltage_factor_chan_b = ui_settings->current_voltage_factor_chan_b;
    oscilloscope_ui->do_plot_current = ui_settings->do_plot_current;

    double a_exp = ceil(log10(ui_settings->current_voltage_factor_chan_a));
    double b_exp = ceil(log10(ui_settings->current_voltage_factor_chan_b));
    double exponent = max(a_exp, b_exp);

    struct Gra_Gridded_Base_Arguments plot_args;
    plot_args.grid_unit_pixels = grid_pixels_unit;
    plot_args.grid_w = 18;
    plot_args.grid_h = 12;
    plot_args.grid_left_axis_off = 2;
    plot_args.grid_bot_axis_off = 2;
    plot_args.grid_skip_x = 1;
    plot_args.grid_skip_y = 1;
    plot_args.x_left = -0.0002f;
    plot_args.x_right = 0.0018f;
    plot_args.y_bot = -0.2f * pow(10.0, exponent);
    plot_args.y_top = 0.8f * pow(10.0, exponent);
    plot_args.x_label = "t [s]";
    plot_args.y_label = !ui_settings->do_plot_current ? "U [V]" : "I [A]";
    plot_args.thick_y_zero = true;
    plot_args.tick_x_label_fmt = "%.6f";
    plot_args.tick_y_label_fmt = "%.2f";
    oscilloscope_ui->plot_args = plot_args;

}

void oscilloscope_change_mode(struct Oscilloscope_State* state, struct Oscilloscope_Ui* ui, const struct Oscilloscope_Settings* settings, bool triggered) {

    if (settings->trigger_mode == triggered) return;

    if (triggered) {
        // TRIGGERED DATA setup
        osc_cleanup_data(state->data);
        osc_triggered_setup(&state->device, &state->data, settings->request_n_samples, &state->n_data, settings->v_pk_to_pk, settings->sample_rate);
        osc_triggered_arm_trigger(&state->device, settings->trigger_timeout, settings->trigger_channel, settings->trigger_level, settings->trigger_position, settings->trigger_type, settings->trigger_condition);
        ui->trigger_armed_timestamp = mui_get_time();
        ui->triggerd_data_aquired = false;
    } else {
        // SHIFT_SCREEN DATA setup
        osc_cleanup_data(state->data);
        osc_shift_screen_setup(&state->device, &state->data, settings->request_n_samples, &state->n_data, settings->v_pk_to_pk, settings->sample_rate);
        ui->trigger_armed_timestamp = mui_get_time() - ui->TRIGGER_ARM_COOLDOWN;
    }
}

struct Internal_Scaled_Data {
    double scale;
    double* x;
};

double internal_scale_data(size_t i, struct Internal_Scaled_Data* data) {
    double scale = data->scale;
    double *x = data->x;
    return x[i] * scale;
}

void oscilloscope_ui_draw(Mui_Rectangle area, float grid_pixel_unit, struct Oscilloscope_Ui* ui, struct Oscilloscope_State* state, const struct Oscilloscope_Settings* settings) {
    Mui_Rectangle scope_settings_area;
    Mui_Rectangle trigger_checkbox_area;
    Mui_Rectangle save_csv_button_area;
    Mui_Rectangle up_button_area;
    Mui_Rectangle down_button_area;

    area = mui_cut_top(area, 1 * grid_pixel_unit, NULL);

    Mui_Rectangle scope_rect = mui_cut_right(area, 5.0f * grid_pixel_unit, &scope_settings_area);

    scope_settings_area = mui_cut_right(scope_settings_area, 1.0f * grid_pixel_unit, NULL);
    scope_settings_area = mui_cut_top(scope_settings_area, 1.0f * grid_pixel_unit, &trigger_checkbox_area);
    scope_settings_area = mui_cut_top(scope_settings_area, 1.0f * grid_pixel_unit, NULL);
    scope_settings_area = mui_cut_top(scope_settings_area, 1.0f * grid_pixel_unit, &save_csv_button_area);

    scope_settings_area = mui_cut_top(scope_settings_area, 2.5f * grid_pixel_unit, NULL);
    scope_settings_area = mui_cut_top(scope_settings_area, 1.0f * grid_pixel_unit, &up_button_area);
    scope_settings_area = mui_cut_top(scope_settings_area, 0.5f * grid_pixel_unit, NULL);
    scope_settings_area = mui_cut_top(scope_settings_area, 1.0f * grid_pixel_unit, &down_button_area);
    down_button_area = mui_cut_right(down_button_area, 2.0f * grid_pixel_unit, NULL);
    up_button_area = mui_cut_right(up_button_area, 2.0f * grid_pixel_unit, NULL);


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

    if (state->device_available || true) {
        if (mui_checkbox(&ui->trigger_armed_cb_state, trigger_label_text, trigger_checkbox_area)) {
            // checkbox toggeled
            bool triggered = ui->trigger_armed_cb_state.checked;
            oscilloscope_change_mode(state, ui, settings, triggered);
        }
        mui_button(&ui->save_csv_btn_state, "Save CSV", save_csv_button_area);
    }

    char* label_inc = ui->do_plot_current?  "A/div inc" : "V/div inc";
    char* label_dec = ui->do_plot_current?  "A/div dec" : "V/div dec";

    if (mui_button(&ui->up_btn_state, label_inc, up_button_area)) {
        ui->plot_args.y_top *= 2;
        ui->plot_args.y_bot *= 2;
    }
    if (mui_button(&ui->down_btn_state, label_dec, down_button_area)) {
        ui->plot_args.y_top *= 0.5f;
        ui->plot_args.y_bot *= 0.5f;
    }

    Mui_Rectangle plot_rect = gra_gridded_xy_base(&ui->plot_args, scope_rect);

    if (!state->device_available) {
        Mui_Vector2 pos = mui_center_of_rectangle(plot_rect);
        char * no_device_text = "Analog discovery oscilloscope not conneted.";
        size_t l = mui_text_len(no_device_text, strlen(no_device_text));
        Mui_Vector2 measure = mui_measure_text(mui_protos_theme_g.label_font, no_device_text,
             mui_protos_theme_g.label_text_size, 0.5f, 0, l);
        pos.x -= measure.x * 0.5f;
        pos.y -= measure.y * 0.5f;
        mui_draw_text_line(mui_protos_theme_g.label_font, pos, 0.5f, mui_protos_theme_g.label_text_size, no_device_text, MUI_RED, 0, l);
    }


    if (!state->device_available) return;

    double t_min = ui->plot_args.x_left;
    double t_max = ui->plot_args.x_right;
    double y_min = ui->plot_args.y_bot;
    double y_max = ui->plot_args.y_top;

    for (int i = 0; i < state->n_display_data; i++) {
        state->display_t_data[i] = t_min + (t_max - t_min) * i / (state->n_display_data - 1);
    }

    // TODO: mui: rename x_resamples to ..._out for consistency
    // TODO: rename display to interpolated
    //mma_spline_cubic_natural(state->display_t_data, state->data, state->n_data, state->display_data, state->display_t_data, state->n_display_data);

    struct Internal_Scaled_Data scaled_data;
    scaled_data.scale = ui->current_voltage_factor_chan_a;
    scaled_data.x = state->data;

    //gra_xy_plot_data_points(state->display_t_data, &scaled_display_data, (void*)&internal_scale_data, state->n_display_data, t_min, t_max, y_min, y_max, MUI_YELLOW, 2.0f, plot_rect);
    //if ( (state->t_max_data - state->t_min_data) / (t_max - t_min) * state->n_display_data / state->n_data > 5 ) {
        gra_xy_plot_data_points(state->t_data, &scaled_data, (void*)&internal_scale_data, state->n_data, t_min, t_max, y_min, y_max, MUI_ORANGE, 2.0f, plot_rect);
    //}

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
