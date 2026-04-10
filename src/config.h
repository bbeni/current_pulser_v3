#ifndef CONFIG_H_
#define CONFIG_H_

// Configuration of hard coded values
// For this to have an effect recompile the program
// This file must only be included once in main.c
// TODO make it dynamic settings in the UI with sensible defaults

const double CONFIG_CURRENT_VOLTAGE_FACTOR_CHANNEL_A = 6072.0;
const double CONFIG_CURRENT_VOLTAGE_FACTOR_CHANNEL_B = 6048.0;
const double CONFIG_VOLTAGE_OFFSET_CHANNEL_A = 0.0084;
const double CONFIG_VOLTAGE_OFFSET_CHANNEL_B = 0.0014;

const double CONFIG_TRIGGER_LEVEL_CURRENT = 200.0; // A

const float CONFIG_REARM_COOLDOWN_SECONDS = 90.0f;

const float CONFIG_MAX_VOLTAGE = 2000.0f;
const float CONFIG_MIN_VOLTAGE = 0.0f;

const float CONFIG_TARGET_CHARGING_CURRENT = 200.0f; // mA

const char CONFIG_DATA_PATH[] = "~/Documents/current_pulser_data";

#endif // CONFIG_H_
