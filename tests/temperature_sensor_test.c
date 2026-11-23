#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "temperature_sensor.h"

static uint8_t registers[256];
static int errors;
static int fail_read_reg = -1;
static int fail_write_reg = -1;
static unsigned int delay_total;
static unsigned int forced_writes;

void Error_Handler(void)
{
    ++errors;
}

void HAL_Delay(uint32_t delay)
{
    delay_total += delay;
}

HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef* i2c, uint16_t address, uint16_t reg, uint16_t reg_size, uint8_t* data, uint16_t size, uint32_t timeout)
{
    assert(i2c != NULL && address == (TEMPERATURE_SENSOR_I2C_ADDRESS << 1));
    assert(reg_size == I2C_MEMADD_SIZE_8BIT && timeout == 100U);
    if (reg == fail_read_reg) {
        return HAL_ERROR;
    }
    memcpy(data, registers + reg, size);
    return HAL_OK;
}

HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef* i2c, uint16_t address, uint16_t reg, uint16_t reg_size, uint8_t* data, uint16_t size, uint32_t timeout)
{
    assert(i2c != NULL && address == (TEMPERATURE_SENSOR_I2C_ADDRESS << 1));
    assert(reg_size == I2C_MEMADD_SIZE_8BIT && timeout == 100U && size == 1U);
    if (reg == fail_write_reg) {
        return HAL_ERROR;
    }
    if (reg == 0xF4U && *data == 0x21U) {
        assert(registers[0xF2] == 1U);
        ++forced_writes;
    }
    registers[reg] = *data;
    return HAL_OK;
}

static void put_u16(unsigned int reg, uint16_t value)
{
    registers[reg] = (uint8_t)value;
    registers[reg + 1U] = (uint8_t)(value >> 8U);
}

static void set_humidity_calibration(int h4, int h5)
{
    unsigned int h4_bits = (unsigned int)h4 & 0xFFFU;
    unsigned int h5_bits = (unsigned int)h5 & 0xFFFU;
    registers[0xE4] = (uint8_t)(h4_bits >> 4U);
    registers[0xE5] = (uint8_t)((h4_bits & 0xFU) | ((h5_bits & 0xFU) << 4U));
    registers[0xE6] = (uint8_t)(h5_bits >> 4U);
}

static void set_raw(uint32_t temperature, uint16_t humidity)
{
    registers[0xFA] = (uint8_t)(temperature >> 12U);
    registers[0xFB] = (uint8_t)(temperature >> 4U);
    registers[0xFC] = (uint8_t)(temperature << 4U);
    registers[0xFD] = (uint8_t)(humidity >> 8U);
    registers[0xFE] = (uint8_t)humidity;
}

static void setup(void)
{
    memset(registers, 0, sizeof(registers));
    registers[0xD0] = 0x60;
    put_u16(0x88, 27504);
    put_u16(0x8A, 26435);
    put_u16(0x8C, (uint16_t)-1000);
    registers[0xA1] = 75;
    put_u16(0xE1, 362);
    set_humidity_calibration(315, 50);
    registers[0xE7] = 30;
    set_raw(519888, 30000);
    errors = 0;
    fail_read_reg = -1;
    fail_write_reg = -1;
    delay_total = 0;
    forced_writes = 0;
}

int main(void)
{
    I2C_HandleTypeDef i2c;
    temperature_sensor_handle_t handle = 0;
    temperature_sensor_data_t data = {123, 456};
    setup();
    assert(temperature_sensor_get_data(handle, &data) == TEMPERATURE_SENSOR_ERROR);
    assert(errors == 1);
    assert(temperature_sensor_init(&handle, &i2c) == TEMPERATURE_SENSOR_OK);
    assert(temperature_sensor_get_data(handle, &data) == TEMPERATURE_SENSOR_OK);
    assert(data.temperature == 2508 && data.humidity == 5429);
    assert(forced_writes == 1 && delay_total >= 10);

    set_raw(519888, 0);
    assert(temperature_sensor_get_data(handle, &data) == TEMPERATURE_SENSOR_OK);
    assert(data.humidity == 0);
    set_raw(519888, 65535);
    assert(temperature_sensor_get_data(handle, &data) == TEMPERATURE_SENSOR_OK);
    assert(data.humidity == 10000);

    set_humidity_calibration(-30, -20);
    registers[0xE7] = (uint8_t)-12;
    assert(temperature_sensor_init(&handle, &i2c) == TEMPERATURE_SENSOR_OK);
    set_raw(519888, 8000);
    assert(temperature_sensor_get_data(handle, &data) == TEMPERATURE_SENSOR_OK);
    assert(data.humidity == 5421);

    set_raw(400000, 8000);
    assert(temperature_sensor_get_data(handle, &data) == TEMPERATURE_SENSOR_OK);
    assert(data.temperature < 0);
    set_raw(0x80000, 8000);
    assert(temperature_sensor_get_data(handle, &data) == TEMPERATURE_SENSOR_OK);
    set_raw(519888, 0x8000);
    assert(temperature_sensor_get_data(handle, &data) == TEMPERATURE_SENSOR_OK);
    data.temperature = 123;
    data.humidity = 456;
    fail_read_reg = 0xFA;
    assert(temperature_sensor_get_data(handle, &data) == TEMPERATURE_SENSOR_ERROR);
    assert(data.temperature == 123 && data.humidity == 456);
    fail_read_reg = -1;
    fail_write_reg = 0xF4;
    assert(temperature_sensor_get_data(handle, &data) == TEMPERATURE_SENSOR_ERROR);
    fail_write_reg = -1;

    registers[0xF3] = 1;
    unsigned int previous_delay = delay_total;
    assert(temperature_sensor_init(&handle, &i2c) == TEMPERATURE_SENSOR_ERROR);
    assert(handle == 0 && delay_total - previous_delay == 100);
    registers[0xF3] = 0;
    registers[0xD0] = 0x58;
    assert(temperature_sensor_init(&handle, &i2c) == TEMPERATURE_SENSOR_ERROR);
    assert(temperature_sensor_get_data(handle, &data) == TEMPERATURE_SENSOR_ERROR);
    assert(temperature_sensor_init(NULL, &i2c) == TEMPERATURE_SENSOR_ERROR);
    assert(temperature_sensor_init(&handle, NULL) == TEMPERATURE_SENSOR_ERROR);
    puts("temperature sensor tests passed");
    return 0;
}
