#ifndef PULSER_CONTROL_H_
#define PULSER_CONTROL_H_

#include <stdbool.h>

//
// pulser inreface
//

//
// state machine for each charge bank:
// MEASUREMENT if voltage of hv power supply good below some threshold
//    -> ERROR / SUCCESS
//    -> BANK 2 SUCCESS is the overall success state
//
typedef enum {
    CHARGE_STATE_READY_FOR_CHARGING,
    CHARGE_STATE_BANK_1_CHARGING,
    CHARGE_STATE_BANK_1_WAITING_FOR_MEASUREMENT,
    CHARGE_STATE_BANK_1_SUCCESS,
    CHARGE_STATE_BANK_1_ERROR, // terminal state
    CHARGE_STATE_BANK_2_CHARGING,
    CHARGE_STATE_BANK_2_WAITING_FOR_MEASUREMENT,
    CHARGE_STATE_BANK_2_SUCCESS, // terminal state
    CHARGE_STATE_BANK_2_ERROR, // terminal state
} CHARGE_STATE;

bool pulser_init();
void pulser_do_fire();
void pulser_do_reset();
CHARGE_STATE pulser_get_charging_state();
bool pulser_update_charge_banks(double target_voltage_1, double target_current_1, double target_voltage_2, double target_current_2);
void pulser_prepare_charging();


//
// high voltage supply interface
//

bool pulser_hv_supply_init();
void pulser_hv_supply_set_voltage_and_current(double voltage, double current);
double pulser_hv_supply_sense_voltage();
double pulser_hv_supply_sense_current();

//
// GPIO interface
//

typedef struct Gpio_Device Gpio_Device; // opaque type

bool gpio_init(Gpio_Device** device, const char* device_identifier);
void gpio_close(Gpio_Device* device);
bool gpio_configure_pin_output(Gpio_Device* device, int pin, int intial_state);
void gpio_set_pin_state(Gpio_Device* device, int pin, int state);
void gpio_sleep(int millis);


//
// pins of raspberry pi
//

#define PIN_ENABLE 17
#define PIN_CHARGE 22
#define PIN_SELECT 23
#define PIN_FIRE 27

struct Pin_State {
    bool enable;
    bool charge;
    bool select;
    bool fire;
};

extern struct Pin_State pulser_pin_state;

// pin 17 enable
// pin 22 charge
// pin 23 select
// pin 27 fire

// i v factor 4000
// v v factor 1000
// wenn stdby (enable off) : select selectes lrc meter
// enable weg -> entladen
// fire relay on - 100 ms - fire relay off -> enable off -> 90s
// ladezeit + 1-2s -> check if sollwert von spannung erreicht vom ladegeraet -> error (entlade relay)
// charge disable, HVsupply aus, spannung von HV messen wird es nach 1s kleiner? -> nein -> error (lade relay)


#endif // PULSER_CONTROL_H_
