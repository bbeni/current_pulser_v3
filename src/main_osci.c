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



int main() {

    int cDevice;
    int cChannel;
    double hzFreq;
    char szDeviceName[32];
    char szSN[32];
    int fIsInUse;
    HDWF hdwf;
    char szError[512];

    // detect connected all supported devices
    if(!FDwfEnum(enumfilterAll, &cDevice)){
        FDwfGetLastErrorMsg(szError);
        printf("FDwfEnum: %s\n", szError);
        return 0;
    }
    // list information about each device
    printf("Found %d devices:\n", cDevice);
    for(int i = 0; i < cDevice; i++){
        // we use 0 based indexing
        FDwfEnumDeviceName (i, szDeviceName);
        FDwfEnumSN(i, szSN);
        printf("\nDevice: %d name: %s %s\n", i+1, szDeviceName, szSN);
        // before opening, check if the device isn’t already opened by other application, like: WaveForms
        FDwfEnumDeviceIsOpened(i, &fIsInUse);
        if(!fIsInUse){
            if(!FDwfDeviceOpen(i, &hdwf)){
                FDwfGetLastErrorMsg(szError);
                printf("FDwfDeviceOpen: %s\n", szError);
                continue;
            }
            FDwfAnalogInChannelCount(hdwf, &cChannel);
            FDwfAnalogInFrequencyInfo(hdwf, NULL, &hzFreq);
            printf("number of analog input channels: %d maximum freq.: %.0f Hz\n", cChannel, hzFreq);
            FDwfDeviceClose(hdwf);
            hdwf = hdwfNone;
        }
    }
    // before application exit make sure to close all opened devices by this process
    FDwfDeviceCloseAll();

    exit(0);
    /*
    # enable all channels
    dwf.FDwfAnalogInChannelEnableSet(device_data.handle, ctypes.c_int(0), ctypes.c_bool(True))

    # set offset voltage (in Volts)
    dwf.FDwfAnalogInChannelOffsetSet(device_data.handle, ctypes.c_int(0), ctypes.c_double(offset))

    # set range (maximum signal amplitude in Volts)
    dwf.FDwfAnalogInChannelRangeSet(device_data.handle, ctypes.c_int(0), ctypes.c_double(amplitude_range))

    # set the buffer size (data point in a recording)
    dwf.FDwfAnalogInBufferSizeSet(device_data.handle, ctypes.c_int(buffer_size))

    # set the acquisition frequency (in Hz)
    dwf.FDwfAnalogInFrequencySet(device_data.handle, ctypes.c_double(sampling_frequency))

    # disable averaging (for more info check the documentation)
    dwf.FDwfAnalogInChannelFilterSet(device_data.handle, ctypes.c_int(-1), constants.filterDecimate)
    data.sampling_frequency = sampling_frequency
    data.buffer_size = buffer_size


     # set up the instrument
    dwf.FDwfAnalogInConfigure(device_data.handle, ctypes.c_bool(False), ctypes.c_bool(False))

    # read data to an internal buffer
    dwf.FDwfAnalogInStatus(device_data.handle, ctypes.c_bool(False), ctypes.c_int(0))

    # extract data from that buffer
    voltage = ctypes.c_double()   # variable to store the measured voltage
    dwf.FDwfAnalogInStatusSample(device_data.handle, ctypes.c_int(channel - 1), ctypes.byref(voltage))

    # store the result as float
    voltage = voltage.value
    */

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