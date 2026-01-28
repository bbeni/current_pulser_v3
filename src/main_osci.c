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


struct Osc_Device {
    HDWF handle;
    double max_freq_hz;
    int n_channels;
    char name[32];
    char serial_number[32];
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


int main() {

    struct Osc_Device device = {0};

    if (! osc_open_device(&device)) {
        osc_print_last_error();
        exit(1);
    }

    FDwfDeviceCloseAll();

    exit(0);

    int grid_w = 18;
    int grid_h = 15;
    int grid_pixel_unit = 50;

    int w, h;
    w = grid_w * grid_pixel_unit;
    h = grid_h * grid_pixel_unit;

    mui_open_window(w, h, 500, 200, "Current Pulser V3 Controller", 1.0f, MUI_WINDOW_RESIZEABLE | MUI_WINDOW_UNDECORATED, NULL);
    mui_init_themes(0, 0, false, "resources/font/NimbusSans-Regular.ttf");


    while (!mui_window_should_close())
    {
        mui_update_core();

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
        gra_xy_plot_labels_and_grid("t [s]", "A [V]", 0, 1, -1, 1, 0.1, 0.2, true, scope_rect);


        mui_end_drawing();
        uti_temp_reset();
    }

    mui_close_window();

    return 0;
}