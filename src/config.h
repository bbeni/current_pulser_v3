#ifndef CONFIG_H_
#define CONFIG_H_

// Configuration of hard coded values
// For this to have an effect recompile the program
// This file must only be included once in main.c
// TODO make it dynamic settings in the UI with sensible defaults

const double CONFIG_CURRENT_VOLTAGE_FACTOR_CHANNEL_A = 6072.0; // A/V
const double CONFIG_CURRENT_VOLTAGE_FACTOR_CHANNEL_B = 6048.0; //  A/V
const double CONFIG_VOLTAGE_OFFSET_CHANNEL_A = 0.0084; // V
const double CONFIG_VOLTAGE_OFFSET_CHANNEL_B = 0.0014; // V

const double CONFIG_TRIGGER_LEVEL_AMPERE = 200.0; // A

const float CONFIG_REARM_COOLDOWN_SECONDS = 90.0f; // s

const float CONFIG_MAX_VOLTAGE = 2000.0f; // V
const float CONFIG_MIN_VOLTAGE = 0.0f; // V

const float CONFIG_TARGET_CHARGING_CURRENT = 200.0f; // mA

const char CONFIG_DATA_PATH[] = "/home/pulser/Documents/current_pulser_data"; // have to give explicit path (no ~)

#endif // CONFIG_H_
