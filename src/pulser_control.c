/*
    This interface should provide access to the raspberry pi pins
*/

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
#include "pulser_control.h"
#include <assert.h>


#define PIN_ENABLE 17
#define PIN_CHARGE 22
#define PIN_SELECT 23
#define PIN_FIRE 27


struct Pin_State pulser_pin_state;;
Gpio_Device* pulser_raspberry_pi;

//
// state machine for both charge banks
//
// RDY -> CHARGING 1 -> WAITING 1s -> MEASUREMENT if voltage of hv power supply good -> ERROR / SUCCESS
//     -> CHARGING 2 -> ...
CHARGE_STATE charge_state;
CHARGE_STATE pulser_get_charging_state() {
    return charge_state;
}

bool pulser_init() {
    if (!gpio_init(&pulser_raspberry_pi, "gpiochip4")) return false;
    if (!gpio_configure_pin_output(pulser_raspberry_pi, PIN_ENABLE, 0)) return false;
    if (!gpio_configure_pin_output(pulser_raspberry_pi, PIN_CHARGE, 0)) return false;
    if (!gpio_configure_pin_output(pulser_raspberry_pi, PIN_SELECT, 0)) return false;
    if (!gpio_configure_pin_output(pulser_raspberry_pi, PIN_FIRE, 0)) return false;
    pulser_pin_state.charge = 0;
    pulser_pin_state.charge = 0;
    pulser_pin_state.charge = 0;
    pulser_pin_state.charge = 0;
    return true;
}


void pulser_do_fire() {
    // fire relay on
    gpio_set_pin_state(pulser_raspberry_pi, PIN_FIRE, 1);
    pulser_pin_state.fire = 1;
    gpio_sleep(1000);
    // fire relay off
    gpio_set_pin_state(pulser_raspberry_pi, PIN_FIRE, 0);
    pulser_pin_state.fire = 0;
    gpio_sleep(1000);
    // enable off
    gpio_set_pin_state(pulser_raspberry_pi, PIN_ENABLE, 0);
    pulser_pin_state.enable = 0;

    charge_state = CHARGE_STATE_READY_FOR_CHARGING;

    // TODO: enforce 90 seconds again
    // 90 s
}

void pulser_do_reset() {
    gpio_sleep(2000);

    gpio_set_pin_state(pulser_raspberry_pi, PIN_FIRE, 0);
    pulser_pin_state.fire = 0;

    gpio_sleep(2000);

    gpio_set_pin_state(pulser_raspberry_pi, PIN_SELECT, 0);
    pulser_pin_state.select = 0;

    gpio_sleep(2000);

    gpio_set_pin_state(pulser_raspberry_pi, PIN_ENABLE, 0);
    pulser_pin_state.enable = 0;

    gpio_sleep(2000);

    gpio_set_pin_state(pulser_raspberry_pi, PIN_CHARGE, 0);
    pulser_pin_state.charge = 0;

    gpio_sleep(2000);

    charge_state = CHARGE_STATE_READY_FOR_CHARGING;
}


void pulser_prepare_charging() {

    // TODO: also make sure other 3 ralays are off

    gpio_set_pin_state(pulser_raspberry_pi, PIN_ENABLE, 1);
    pulser_pin_state.enable = 1;
    gpio_sleep(100);
    gpio_set_pin_state(pulser_raspberry_pi, PIN_ENABLE, 1);
    pulser_pin_state.enable = 1;
    gpio_sleep(100);

    charge_state = CHARGE_STATE_READY_FOR_CHARGING;
}


// TODO remove me
#ifdef FAKE_
double fake_hv_voltage = 0.0;
double fake_hv_current = 0.0;
double fake_hv_voltage_setpoint = 0.0;

bool pulser_hv_supply_init() {
    return true;
}

void pulser_hv_supply_set_voltage_and_current(double voltage, double current) {
    // TODO: implement
    fake_hv_current = current;
    fake_hv_voltage_setpoint = voltage;
    if (voltage < 10) {
        fake_hv_voltage_setpoint = 0;
        fake_hv_voltage = 0;
    }
}

double pulser_hv_supply_sense_voltage() {
    // TODO: implement
    if (fake_hv_voltage <= fake_hv_voltage_setpoint) fake_hv_voltage += 0.5;
    return fake_hv_voltage;
}

double pulser_hv_supply_sense_current() {
    return fake_hv_current;
}

#endif // FAKE_



// return true when finished
bool pulser_update_charge_banks(double target_voltage_1, double target_current_1, double target_voltage_2, double target_current_2) {

    switch (charge_state) {

    case CHARGE_STATE_READY_FOR_CHARGING:
        // TODO: enable ON

        // select bank 1
        gpio_set_pin_state(pulser_raspberry_pi, PIN_SELECT, 0);
        pulser_pin_state.select = 0;
        gpio_sleep(100);
        gpio_set_pin_state(pulser_raspberry_pi, PIN_CHARGE, 1);
        pulser_pin_state.charge = 1;
        gpio_sleep(100);
        pulser_hv_supply_set_voltage_and_current(target_voltage_1, target_current_1);
        gpio_sleep(100);
        pulser_hv_supply_output_on();
        charge_state = CHARGE_STATE_BANK_1_CHARGING;
    break;
    case CHARGE_STATE_BANK_1_CHARGING:
        // update charging process
        double measured_voltage = pulser_hv_supply_sense_voltage();
        // TODO add small reserve voltage
        if (measured_voltage > target_voltage_1) {
            gpio_set_pin_state(pulser_raspberry_pi, PIN_CHARGE, 0);
            pulser_pin_state.charge = 0;
            // wait for the relay
            gpio_sleep(100);
            // deactivate hv supply
            pulser_hv_supply_output_off();
            charge_state = CHARGE_STATE_BANK_1_WAITING_FOR_MEASUREMENT;
        }
    break;
    case CHARGE_STATE_BANK_1_WAITING_FOR_MEASUREMENT:
        // wait 1 second
        gpio_sleep(1000);
        // measure hv supply voltage
        double measured = pulser_hv_supply_sense_voltage();
        // TODO: factor out 1 V value
        if (measured >= 1.0) {
            // TODO: enable weg
            charge_state = CHARGE_STATE_BANK_1_ERROR;
        } else {
            charge_state = CHARGE_STATE_BANK_1_SUCCESS;
        }
    break;
    case CHARGE_STATE_BANK_1_ERROR:
        pulser_hv_supply_output_off();
        return true;
    break;
    case CHARGE_STATE_BANK_1_SUCCESS:
        // select bank 2
        gpio_set_pin_state(pulser_raspberry_pi, PIN_SELECT, 1);
        pulser_pin_state.select = 1;
        gpio_sleep(100);
        gpio_set_pin_state(pulser_raspberry_pi, PIN_CHARGE, 1);
        pulser_pin_state.charge = 1;
        gpio_sleep(100);
        pulser_hv_supply_set_voltage_and_current(target_voltage_2, target_current_2);
        gpio_sleep(100);
        pulser_hv_supply_output_on();
        charge_state = CHARGE_STATE_BANK_2_CHARGING;
    break;
    case CHARGE_STATE_BANK_2_CHARGING:
        // update charging process
        double voltage = pulser_hv_supply_sense_voltage();
        // TODO: add safty margin
        if (voltage > target_voltage_2) {
            gpio_set_pin_state(pulser_raspberry_pi, PIN_CHARGE, 0);
            pulser_pin_state.charge = 0;
            // wait for the relay
            gpio_sleep(100);
            // deactivate hv supply
            pulser_hv_supply_output_off();
            charge_state = CHARGE_STATE_BANK_2_WAITING_FOR_MEASUREMENT;
        }
    break;
    case CHARGE_STATE_BANK_2_WAITING_FOR_MEASUREMENT:
        // wait 1 second
        gpio_sleep(1000);
        // measure hv supply voltage
        double measured_volts = pulser_hv_supply_sense_voltage();
        // TODO: factor out 1 V value
        if (measured_volts >= 1.0) {
            // TODO: enable off
            charge_state = CHARGE_STATE_BANK_2_ERROR;
        } else {
            charge_state = CHARGE_STATE_BANK_2_SUCCESS;
        }
    break;
    case CHARGE_STATE_BANK_2_SUCCESS:
        return true;
    break;
    case CHARGE_STATE_BANK_2_ERROR:
        pulser_hv_supply_output_off();
        return true;
    default:
        assert(false && "CHARGE_STATE overflow");
    }

    // not finished
    return false;
}




