#include "pulser_control.h"

// TODO remove me
#ifndef FAKE_

#include <modbus.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

// Registers mapping (from Python script)
#define COIL_PC        0x0500
#define REG_CMD        0x0A00
#define REG_VMAX       0x0A01
#define REG_IMAX       0x0A03
#define REG_VSET       0x0A05
#define REG_ISET       0x0A07
#define REG_VS         0x0B00
#define REG_IS         0x0B02


// CMD Values
#define CMD_SET_VOLTAGE 1
#define CMD_SET_CURRENT 2

// Global/Static Modbus context
static modbus_t *ctx = NULL;

static void float_to_u16(float val, uint16_t *regs) {
    uint32_t i;
    memcpy(&i, &val, sizeof(i)); // Safe type punning
    regs[0] = (uint16_t)((i >> 16) & 0xFFFF); // High word
    regs[1] = (uint16_t)(i & 0xFFFF);         // Low word
    // Note: Swap regs[0] and regs[1] if the DSC Power Supply
    // expects Little-Endian word order (CDAB instead of ABCD).
}

static float u16_to_float(const uint16_t *regs) {
    uint32_t i = ((uint32_t)regs[0] << 16) | regs[1];
    float val;
    memcpy(&val, &i, sizeof(val));
    return val;
}

bool pulser_hv_supply_init() {
    ctx = modbus_new_rtu("/dev/ttyUSB0", 9600, 'N', 8, 1);

    if (ctx == NULL) {
        printf("ERROR: Unable to create the libmodbus context\n");
        return false;
    }

    modbus_set_slave(ctx, 1);

    modbus_set_response_timeout(ctx, 10, 0);

    if (modbus_connect(ctx) == -1) {
        printf("ERROR: Connection failed: %s\n", modbus_strerror(errno));
        modbus_free(ctx);
        ctx = NULL;
        return false;
    }

    // 1. Enable Remote Control (Write Coil PC)
    if (modbus_write_bit(ctx, COIL_PC, TRUE) == -1) {
        printf("ERROR: Failed to enable remote control.\n");
        return false;
    }

    // 2. Setup initial safety limits based on python setup_power_supply()
    uint16_t regs[2];
    float_to_u16(0.180f, regs);
    modbus_write_registers(ctx, REG_IMAX, 2, regs);
    modbus_write_register(ctx, REG_CMD, CMD_SET_CURRENT);

    // Set Voltage limit to 2100 V
    float_to_u16(2100.0f, regs);
    modbus_write_registers(ctx, REG_VMAX, 2, regs);
    modbus_write_register(ctx, REG_CMD, CMD_SET_VOLTAGE);

    // TODO rename hv supply with model
    // TODO in general make explicit what hardware we are
    printf("INFO: hv supply init successful\n");

    return true;
}

void pulser_hv_supply_set_voltage_and_current(double voltage, double current) {
    if (ctx == NULL) return;

    uint16_t regs[2];

    // 1. Set the Current (ISET)
    float_to_u16((float)current, regs);
    modbus_write_registers(ctx, REG_ISET, 2, regs);
    // Send CMD to apply Current Set
    modbus_write_register(ctx, REG_CMD, CMD_SET_CURRENT);

    // 2. Set the Voltage (VSET)
    float_to_u16((float)voltage, regs);
    modbus_write_registers(ctx, REG_VSET, 2, regs);
    // Send CMD to apply Voltage Set
    modbus_write_register(ctx, REG_CMD, CMD_SET_VOLTAGE);
}

void pulser_hv_supply_output_on() {
    modbus_write_register(ctx, REG_CMD, 6);
}

void pulser_hv_supply_output_off() {
    modbus_write_register(ctx, REG_CMD, 7);
}

double pulser_hv_supply_sense_voltage() {
    if (ctx == NULL) return 0.0;

    uint16_t regs[2] = {0, 0};

    // Read 2 holding registers starting at VS (0x0B00)
    int rc = modbus_read_registers(ctx, REG_VS, 2, regs);
    if (rc == -1) {
        fprintf(stderr, "Failed to read voltage: %s\n", modbus_strerror(errno));
        return 0.0;
    }

    return (double)u16_to_float(regs);
}

double pulser_hv_supply_sense_current() {
    if (ctx == NULL) return 0.0;

    uint16_t regs[2] = {0, 0};

    // Read 2 holding registers starting at IS (0x0B02)
    int rc = modbus_read_registers(ctx, REG_IS, 2, regs);
    if (rc == -1) {
        fprintf(stderr, "Failed to read current: %s\n", modbus_strerror(errno));
        return 0.0;
    }

    return (double)u16_to_float(regs);
}

void pulser_hv_supply_close() {
    if (ctx != NULL) {
        modbus_close(ctx);
        modbus_free(ctx);
        ctx = NULL;
    }
}

#endif // FAKE_
