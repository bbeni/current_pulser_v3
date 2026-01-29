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
void osc_create_measurement(struct Osc_Device* device, double** data_out, int* n_samples_out) {

    // enable channels
    for(int c = 0; c < device->n_channels; c++){
        FDwfAnalogInChannelEnableSet(device->handle, c, true);
    }
    // set 5V pk2pk input range for all channels
    FDwfAnalogInChannelRangeSet(device->handle, -1, 5);

    // 20MHz sample rate
    FDwfAnalogInFrequencySet(device->handle, 20000000.0);

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
void osc_shift_screen_setup(struct Osc_Device* device, double** data_out, int* n_samples_out) {

    // enable channels
    for(int c = 0; c < device->n_channels; c++){
        FDwfAnalogInChannelEnableSet(device->handle, c, true);
    }
    // set 5V pk2pk input range for all channels
    FDwfAnalogInChannelRangeSet(device->handle, -1, 5);

    // 20MHz sample rate
    FDwfAnalogInFrequencySet(device->handle, 20000000.0);

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

    /*
    int n_valid_samples;
    FDwfDigitalInStatusSamplesValid(device->handle, &n_valid_samples);
    printf("number of valid samples: %d\n", n_valid_samples);

    // scan bar
    int bar_index;
    FDwfDigitalInStatusIndexWrite(device->handle, &bar_index);
    printf("scan bar index: %d\n", bar_index);
    */
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
void osc_triggered_setup(struct Osc_Device* device, double** data_out, int* n_samples_out) {

    // enable channels
    for(int c = 0; c < device->n_channels; c++){
        FDwfAnalogInChannelEnableSet(device->handle, c, true);
    }
    // set 5V pk2pk input range for all channels
    FDwfAnalogInChannelRangeSet(device->handle, -1, 5);

    // 20MHz sample rate
    FDwfAnalogInFrequencySet(device->handle, 20000000.0);

    // get the maximum buffer size
    FDwfAnalogInBufferSizeInfo(device->handle, NULL, n_samples_out);
    FDwfAnalogInBufferSizeSet(device->handle, *n_samples_out);
    FDwfAnalogInAcquisitionModeSet(device->handle, acqmodeSingle);

    double* data;
    data = malloc(sizeof(*data) * *n_samples_out);
    memset(data, 0x0, sizeof(*data) * *n_samples_out);

    *data_out = data;

    // start
    FDwfAnalogInConfigure(device->handle, 0, true);
}

// wait at least 2 seconds with Analog Discovery for the offset to stabilize, before the first reading after device open or offset/range change
void osc_triggered_arm_trigger(struct Osc_Device* device, float time_out, int channel, OSC_TRIGGER_TYPE trigger_type, float trigger_level, OSC_TRIGGER_CONDITION trigger_condition) {
    // configure trigger
    FDwfAnalogInTriggerSourceSet(device->handle, trigsrcDetectorAnalogIn);
    FDwfAnalogInTriggerAutoTimeoutSet(device->handle, time_out);
    FDwfAnalogInTriggerChannelSet(device->handle, channel);
    FDwfAnalogInTriggerTypeSet(device->handle, trigger_type);
    FDwfAnalogInTriggerLevelSet(device->handle, trigger_level);
    FDwfAnalogInTriggerConditionSet(device->handle, trigger_condition);
    device->triggered_measurement_started = false;
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


int main() {

    struct Osc_Device device = {0};

    if (!osc_open_device(&device)) {
        osc_print_last_error();
        exit(1);
    }

    double* data;
    int n_data;
    //osc_create_measurement(&device, &data, &n_data);
    osc_shift_screen_setup(&device, &data, &n_data);
    //osc_triggered_setup(&device, &data, &n_data);


    printf("setup %d data_point measurement\n", n_data);

    //
    // setup arrays for data interpolation
    //
    double step = 1.0 / device.max_freq_hz;
    double end = n_data * step;
    size_t n_interpol = 2000;
    double stretch = 1.0f;
    double interp_step = end * stretch / n_interpol;

    double* t_data;
    double* t_space;
    double* data_interpolated;

    t_data = malloc(sizeof(*t_data) * n_data);
    data_interpolated = malloc(sizeof(double) * n_interpol);
    t_space = malloc(sizeof(double) * n_interpol);

    for (int i = 0; i < n_data; i++) {
        t_data[i] = i * step;
    }
    for (size_t i = 0; i < n_interpol; i++) {
        t_space[i] = i * interp_step;
    }


    // ui stuff
    int grid_w = 18;
    int grid_h = 15;
    int grid_pixel_unit = 50;

    int w, h;
    w = grid_w * grid_pixel_unit;
    h = grid_h * grid_pixel_unit;

    mui_open_window(w, h, 500, 200, "Current Pulser V3 Controller", 1.0f, MUI_WINDOW_RESIZEABLE /* | MUI_WINDOW_UNDECORATED */, NULL);
    mui_init_themes(0, 0, false, "resources/font/NimbusSans-Regular.ttf");

    Mui_Checkbox_State trigger_armed_cb_state = {0};
    const float TRIGGER_ARM_COOLDOWN = 2.0f;
    float trigger_armed_timestamp = -10000.0f;
    bool triggerd_data_aquired = false;


    while (!mui_window_should_close())
    {
        mui_update_core();

        float trigger_armed_cooldown = TRIGGER_ARM_COOLDOWN + trigger_armed_timestamp - mui_get_time();

        if (trigger_armed_cb_state.checked) {
            if (!triggerd_data_aquired) {
                if (osc_triggered_update(&device, trigger_armed_cooldown, data, n_data)) {
                    triggerd_data_aquired = true;
                }
            }
        } else {
            osc_shift_screen_update(&device, data, n_data);
        };

        // TODO: mui: rename x_resamples to ..._out for consistency
        mma_spline_cubic_natural(t_data, data, n_data, data_interpolated, t_space, n_interpol);

        w = mui_screen_width();
        h = mui_screen_height();

        mui_begin_drawing();
        mui_clear_background(mui_protos_theme_g.bg_light, NULL);

        Mui_Rectangle whole_screen = mui_rectangle(0, 0, w, h);

        const float decoration_height = grid_pixel_unit;
        Mui_Rectangle menu_bar_area = mui_window_decoration(decoration_height, true, true, true, true, true, whole_screen);
        Mui_Rectangle screen = mui_cut_top(whole_screen, decoration_height, NULL);
        mui_label(&mui_protos_theme_g, "PULSER V3 OSCILLOSCOPE", MUI_TEXT_ALIGN_DEFAULT, menu_bar_area);

        Mui_Rectangle scope_rect = mui_shrink(screen, grid_pixel_unit);

        Mui_Rectangle trigger_menu_bar_rect;
        Mui_Rectangle trigger_menu_rect;
        scope_rect = mui_cut_top(scope_rect, 1 * grid_pixel_unit, &trigger_menu_bar_rect);
        scope_rect = mui_cut_top(scope_rect, 1 * grid_pixel_unit, NULL);

        mui_cut_right(trigger_menu_bar_rect, 5 * grid_pixel_unit, &trigger_menu_rect);


        char trigger_label_text[40];
        if (trigger_armed_cooldown > 0) {
            snprintf(trigger_label_text, 39, "TRIGGER (%.1f s)", trigger_armed_cooldown);
        } else {
            if (trigger_armed_cb_state.checked) {
                snprintf(trigger_label_text, 39, "TRIGGER (armed)");
            } else {
                snprintf(trigger_label_text, 39, "TRIGGER");
            }
        }

        if (mui_checkbox(&trigger_armed_cb_state, trigger_label_text, trigger_menu_rect)) {
            // checkbox toggeled
            if (trigger_armed_cb_state.checked) {
                // TRIGGERED DATA setup
                osc_cleanup_data(data);
                osc_triggered_setup(&device, &data, &n_data);
                osc_triggered_arm_trigger(&device, 1.0f, 0, OSC_TRIGGER_TYPE_EDGE, 2.0f, OSC_TRIGGER_CONDITION_RISING_POSITIVE);
                trigger_armed_timestamp = mui_get_time();
                triggerd_data_aquired = false;
            } else {
                // SHIFT_SCREEN DATA setup
                osc_cleanup_data(data);
                osc_shift_screen_setup(&device, &data, &n_data);
            }
        }

        Mui_Rectangle plot_rect = gra_xy_plot_labels_and_grid("t [s]", "A [V]", 0, end, -0.1, 0.1, end / 8, 0.02, true, scope_rect);


        gra_xy_plot_data_points(t_space, data_interpolated, NULL, n_interpol, 0, end * stretch, -0.05, 0.05, MUI_YELLOW, 1.0f, plot_rect);


        mui_end_drawing();
        uti_temp_reset();
    }

    FDwfDeviceClose(device.handle);
    mui_close_window();

    return 0;
}