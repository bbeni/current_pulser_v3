#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#ifndef _WIN
// stub implementation
//#include <Windows.h>
#include <unistd.h>

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

void gpio_sleep(int millis) {
    //Sleep(millis);
    usleep(millis * 1000);

}

#else // not windows so __LINUX__ ???

#include <unistd.h>
#include <gpiod.h>
#define CONSUMER "Pulser Platform GPIO"

struct Gpio_Device {
    #define ERROR_LEN 128
    char last_err_msg[ERROR_LEN];

    struct gpiod_chip *chip;
    #define LINES_LEN 256
    struct gpiod_line *lines_array[LINES_LEN];
};


// initializes the gpio device (opaque)
// error message is stored inside device
// device_identifier is "gpiochip4" for raspberry 4 or 5 for example
bool gpio_init(struct Gpio_Device** device, const char* device_identifier) {
    /*

    struct Gpio_Device* dev_;
    dev_ = malloc(sizeof(*dev_));
    assert(dev_ != NULL && "FATAL ERROR: malloc failed here..");

    *device = dev_;

    dev_->chip = gpiod_chip_open_by_name(device_identifier);

    for (int i = 0; i < LINES_LEN; i++) {
        dev_->lines_array[i] = NULL;
    }

    if (!chip) {
        char err_msg[] = "Failed to open the device chip '%s'";
        printf(err_msg, device_identifier);
        snprintf(dev_->last_err_msg, ERROR_LEN, err_msg, device_identifier);
        return false;
    }
    */
    return true;
}

void gpio_close(struct Gpio_Device* device) {
    /*
    for (int i = 0; i < LINES_LEN; i++) {
        if (device->lines_array[i] != NULL) {
            gpiod_line_release(device->lines_array[i]);
        }
    }
    gpiod_chip_close(device->chip);
*/
}

// intital_state can be LOW or HIGH
bool gpio_configure_pin_output(struct Gpio_Device* device, int pin, int initial_state) {
 /*
    assert(pin < LINES_LEN && "increase LINES_LEN it is a static array!");

    if (device->lines_array[pin]) {
        device->lines_array[pin] = gpiod_chip_get_line(chip, pin);
        if (device->lines_array[pin] == NULL) {
            char err_msg[] = "Failed to open the pin '%d'";
            printf(err_msg, pin);
            snprintf(dev_->last_err_msg, ERROR_LEN, err_msg, pin);
            return false;
        }
    }

    if (gpiod_line_request_output(device->lines_array[pin], CONSUMER, initial_state) < 0) {
        char err_msg[] = "Failed to reques the pin '%d' as an output";
        printf(err_msg, pin);
        snprintf(device->last_err_msg, ERROR_LEN, err_msg, pin);
        return false;
    }
    */
    return true;
}

// set the pin to the value of state
void gpio_set_pin_state(struct Gpio_Device* device, int pin, int state) {

/*
    assert(pin < LINES_LEN && "increase LINES_LEN it is a static array!");

    if (device->lines_array[pin]) {
        // TODO: error check
        // gpiod_line_request_set_value(device->lines_array[pin], 0, state);
    } else {
        printf("ERROR: pin not setup here %d", pin);
    }
*/
}

void gpio_sleep(int millis) {
    usleep(millis * 1000);
}


#endif //__linux__
