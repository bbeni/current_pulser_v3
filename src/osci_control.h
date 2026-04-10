// Copyright (C) 2026 Benjamin Froelich
// This file is part of https://github.com/bbeni/current_pulser_v3
// For conditions of distribution and use, see copyright notice in project root.
#ifndef OSCI_CONTROL_
#define OSCI_CONTROL_

#include "gra.h"

typedef enum {
    OSC_TRIGGER_TYPE_EDGE = 0, // trigtypeEdge,
    OSC_TRIGGER_TYPE_PULSE = 1, // trigtypePulse,
    OSC_TRIGGER_TYPE_TRANSITION = 2, // trigtypeTransition,
    OSC_TRIGGER_TYPE_WINDOW = 3, // trigtypeWindow,
} OSC_TRIGGER_TYPE;


typedef enum {
    OSC_TRIGGER_CONDITION_RISING_POSITIVE  = 0, // trigcondRisingPositive   = 0;
    OSC_TRIGGER_CONDITION_FALLING_NEGATIVE = 1, // trigcondFallingNegative  = 1;
} OSC_TRIGGER_CONDITION;


//
// platform spcific (Here Analog discovery)
//
typedef int HDWF;
typedef unsigned char STS;

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

void osc_print_last_error();
bool osc_open_device(struct Osc_Device* device);
void osc_cleanup_data(double* data);


bool osc_shift_screen_setup(struct Osc_Device* device, double** data_out, int request_n_samples, int* n_samples_out, int n_channels, double v_pk_to_pk, double sample_rate);
bool osc_shift_screen_update(struct Osc_Device* device, double* data_out, int n_samples, int n_channels);

bool osc_triggered_setup(struct Osc_Device* device, double** data_out, int request_n_samples, int* n_samples_out, int n_channels, double v_pk_to_pk, double sample_rate);
bool osc_triggered_arm_trigger(struct Osc_Device* device, double auto_time_out, int channel, double level, double position, double trig_length, OSC_TRIGGER_TYPE type, OSC_TRIGGER_CONDITION condition);
bool osc_triggered_update(struct Osc_Device* device, float trigger_cooldown, double* data_out, int n_samples, int n_channels);

//
// Higher level API
//


struct Oscilloscope_State {
    struct Osc_Device device;
    bool device_available;

    double* data;
    double* t_data;
    int n_data;
    int n_channels;

    double* y_data_interpolated;
    double* t_data_interpolated;
    int n_interpol;

    // derived
    double step;
    double t_total;
    double t_min_data;
    double t_max_data;
};

struct Oscilloscope_Ui {
    Mui_Checkbox_State trigger_armed_cb_state;
    Mui_Button_State up_btn_state;
    Mui_Button_State down_btn_state;

    float trigger_armed_timestamp;
    bool triggerd_data_aquired;

    float TRIGGER_ARM_COOLDOWN;

    struct Gra_Gridded_Base_Arguments plot_args;

    bool do_plot_current;
    double current_voltage_factor_chan_a;
    double current_voltage_factor_chan_b;
    double voltage_offset_chan_a;
    double voltage_offset_chan_b;
};

struct Oscilloscope_Settings {
    double sample_rate;
    int request_n_samples; // -1 for maximum amount of samples
    int n_channels;
    double v_pk_to_pk;

    bool trigger_mode; // either that or shift window
    int trigger_channel;
    double trigger_level; // volts
    double trigger_position; // in seconds
    double trigger_auto_timeout; // in seconds
    double trigger_length; // in seconds
    OSC_TRIGGER_TYPE trigger_type;
    OSC_TRIGGER_CONDITION trigger_condition;
};

struct Oscilloscope_Ui_Settings {
    bool do_plot_current;
    double current_voltage_factor_chan_a;
    double current_voltage_factor_chan_b;
    double voltage_offset_chan_a;
    double voltage_offset_chan_b;
};


bool oscilloscope_setup(struct Oscilloscope_State* state, const struct Oscilloscope_Settings* settings);
void oscilloscope_ui_setup(struct Oscilloscope_Ui* oscilloscope_ui, const struct Oscilloscope_Ui_Settings* ui_settings, float grid_pixels_unit);
void oscilloscope_change_mode(struct Oscilloscope_State* state, struct Oscilloscope_Ui* ui, const struct Oscilloscope_Settings* settings, bool triggered);
void oscilloscope_ui_draw(Mui_Rectangle area, float grid_pixel_unit, struct Oscilloscope_Ui* ui, struct Oscilloscope_State* state, const struct Oscilloscope_Settings* settings);
void oscilloscope_ui_update(struct Oscilloscope_Ui* oscilloscope_ui, struct Oscilloscope_State* oscilloscope_state);
void oscilloscope_destroy(struct Oscilloscope_State* oscilloscope_state);

void oscilloscope_generate_csv_filepath(char* buffer, size_t n);
void oscilloscope_save_csv(const char* path, struct Oscilloscope_State* state, struct Oscilloscope_Ui* ui);


#endif // OSCI_CONTROL_
