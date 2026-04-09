#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "pulser_control.h"

#ifndef _WIN
// stub implementation
//#include <Windows.h>

bool gpio_init(struct Gpio_Device** device, const char* device_identifier) {
    (void) device;
    printf("gpio_init %s\n", device_identifier);
    return true;
}

void gpio_close(struct Gpio_Device* device) {
    (void) device;
    printf("gpio_close\n");
}

// intital_state can be LOW or HIGH
bool gpio_configure_pin_output(struct Gpio_Device* device, int pin, int intial_state) {
    (void) device;
    printf("gpio_configure_pin_output pin %d initial_state %d\n", pin, intial_state);
    return true;
}

// set the pin to the value of state
void gpio_set_pin_state(struct Gpio_Device* device, int pin, int state) {
    (void) device;
    printf("gpio_set_pin_state pin %d state %d\n", pin, state);
}

void gpio_sleep(uint32_t millis) {
    //Sleep(millis);
    exit(111);
}

#else // not windows so __LINUX__ ???

#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <gpiod.h>

#define CONSUMER "Pulser Platform GPIO"

struct Gpio_Device {
    #define ERROR_LEN 128
    char last_err_msg[ERROR_LEN];

    struct gpiod_chip *chip;
    #define LINES_LEN 256
    // v2 uses line requests rather than raw line structs
    struct gpiod_line_request *requests_array[LINES_LEN];
};

// initializes the gpio device (opaque)
// error message is stored inside device
// device_identifier is "gpiochip4" or "/dev/gpiochip4"
bool gpio_init(struct Gpio_Device** device, const char* device_identifier) {
    struct Gpio_Device* dev_ = malloc(sizeof(*dev_));
    assert(dev_ != NULL && "FATAL ERROR: malloc failed here..");

    *device = dev_;

    // Handle v1 vs v2 pathing gracefully. v2 expects full paths like "/dev/gpiochip4"
    char path[256];
    if (strncmp(device_identifier, "/dev/", 5) != 0) {
        snprintf(path, sizeof(path), "/dev/%s", device_identifier);
    } else {
        strncpy(path, device_identifier, sizeof(path));
    }

    dev_->chip = gpiod_chip_open(path);

    for (int i = 0; i < LINES_LEN; i++) {
        dev_->requests_array[i] = NULL;
    }

    if (!dev_->chip) {
        char err_msg[] = "Failed to open the device chip '%s'";
        printf(err_msg, path);
        printf("\n");
        snprintf(dev_->last_err_msg, ERROR_LEN, err_msg, path);
        return false;
    }

    return true;
}

void gpio_close(struct Gpio_Device* device) {
    if (!device) return;

    for (int i = 0; i < LINES_LEN; i++) {
        if (device->requests_array[i] != NULL) {
            gpiod_line_request_release(device->requests_array[i]);
        }
    }

    if (device->chip) {
        gpiod_chip_close(device->chip);
    }

    free(device);
}

// intital_state can be LOW or HIGH (0 or 1)
bool gpio_configure_pin_output(struct Gpio_Device* device, int pin, int initial_state) {
    assert(pin < LINES_LEN && "increase LINES_LEN it is a static array!");
    if (!device || !device->chip) return false;

    // If pin is already requested, release it first to avoid busy errors
    if (device->requests_array[pin] != NULL) {
        gpiod_line_request_release(device->requests_array[pin]);
        device->requests_array[pin] = NULL;
    }

    unsigned int offset = (unsigned int)pin;

    // 1. Setup line settings
    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    if (!settings) return false;

    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_output_value(settings, initial_state ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE);

    // 2. Map settings to our specific pin
    struct gpiod_line_config *line_cfg = gpiod_line_config_new();
    gpiod_line_config_add_line_settings(line_cfg, &offset, 1, settings);

    // 3. Setup consumer identity
    struct gpiod_request_config *req_cfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req_cfg, CONSUMER);

    // 4. Request the line
    struct gpiod_line_request *req = gpiod_chip_request_lines(device->chip, req_cfg, line_cfg);

    // 5. Cleanup the setup structs (they are no longer needed after request)
    gpiod_line_settings_free(settings);
    gpiod_line_config_free(line_cfg);
    gpiod_request_config_free(req_cfg);

    if (!req) {
        char err_msg[] = "Failed to request the pin '%d' as an output";
        printf(err_msg, pin);
        printf("\n");
        snprintf(device->last_err_msg, ERROR_LEN, err_msg, pin);
        return false;
    }

    device->requests_array[pin] = req;
    return true;
}

// set the pin to the value of state
void gpio_set_pin_state(struct Gpio_Device* device, int pin, int state) {
    assert(pin < LINES_LEN && "increase LINES_LEN it is a static array!");
    if (!device) return;

    if (device->requests_array[pin]) {
        gpiod_line_request_set_value(
            device->requests_array[pin],
            pin,
            state ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE
        );
    } else {
        printf("ERROR: pin not setup here %d\n", pin);
    }
}

void gpio_sleep(uint32_t millis) {
    uint64_t nanos = (millis * 1000000 );
    struct timespec t;
    t.tv_sec  = nanos / 1000000000;
    t.tv_nsec = nanos % 1000000000;
    nanosleep(&t, NULL);
}

#endif //__linux__