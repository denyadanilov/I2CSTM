#include "temperature_sensor.h"

#include <limits.h>
#include <stddef.h>

#include "main.h"

#define SENSOR_I2C_ADDRESS_SDO_LOW 0x76U
#define SENSOR_I2C_ADDRESS_SDO_HIGH 0x77U

#ifndef TEMPERATURE_SENSOR_I2C_ADDRESS
#define TEMPERATURE_SENSOR_I2C_ADDRESS SENSOR_I2C_ADDRESS_SDO_LOW
#endif

#if TEMPERATURE_SENSOR_I2C_ADDRESS != SENSOR_I2C_ADDRESS_SDO_LOW && TEMPERATURE_SENSOR_I2C_ADDRESS != SENSOR_I2C_ADDRESS_SDO_HIGH
#error "BME280 I2C address must be 0x76 or 0x77"
#endif

#define SENSOR_I2C_TIMEOUT_MS 100U
#define SENSOR_READY_TIMEOUT_MS 100U
#define SENSOR_READY_POLL_INTERVAL_MS 1U
#define SENSOR_MEASUREMENT_TIME_MS 10U
#define SENSOR_HANDLE_VALUE 1
#define SENSOR_INVALID_HANDLE 0
#define SENSOR_REG_CHIP_ID 0xD0U
#define SENSOR_CHIP_ID 0x60U
#define SENSOR_REG_TEMP_CALIB 0x88U
#define SENSOR_REG_HUM_CALIB_1 0xA1U
#define SENSOR_REG_HUM_CALIB_2 0xE1U
#define SENSOR_REG_CTRL_HUM 0xF2U
#define SENSOR_REG_STATUS 0xF3U
#define SENSOR_REG_CTRL_MEAS 0xF4U
#define SENSOR_REG_DATA 0xFAU
#define SENSOR_STATUS_BUSY 0x09U
#define SENSOR_SLEEP_MODE 0x00U
#define SENSOR_HUM_OVERSAMPLING_1 0x01U
#define SENSOR_FORCED_TEMP_OVERSAMPLING_1 0x21U

#define SENSOR_TEMP_CALIB_SIZE 6U
#define SENSOR_HUM_CALIB_SIZE 7U
#define SENSOR_DATA_SIZE 5U
#define SENSOR_TEMP_CALIB_T2_OFFSET 2U
#define SENSOR_TEMP_CALIB_T3_OFFSET 4U
#define SENSOR_HUM_CALIB_H3_OFFSET 2U
#define SENSOR_HUM_CALIB_H4_MSB_OFFSET 3U
#define SENSOR_HUM_CALIB_H4_H5_LSB_OFFSET 4U
#define SENSOR_HUM_CALIB_H5_MSB_OFFSET 5U
#define SENSOR_HUM_CALIB_H6_OFFSET 6U
#define SENSOR_DATA_TEMP_MSB_OFFSET 0U
#define SENSOR_DATA_TEMP_LSB_OFFSET 1U
#define SENSOR_DATA_TEMP_XLSB_OFFSET 2U
#define SENSOR_DATA_HUM_MSB_OFFSET 3U
#define SENSOR_DATA_HUM_LSB_OFFSET 4U
#define SENSOR_BYTE_BITS 8U
#define SENSOR_NIBBLE_BITS 4U
#define SENSOR_TEMP_MSB_SHIFT 12U
#define SENSOR_NIBBLE_MASK 0x0FU
#define SENSOR_SIGN_BIT_8 0x80U
#define SENSOR_SIGN_BIT_12 0x0800U
#define SENSOR_SIGN_BIT_16 0x8000U

#define SENSOR_TEMP_ADC_LINEAR_DIVISOR 16384.0
#define SENSOR_TEMP_T1_LINEAR_DIVISOR 1024.0
#define SENSOR_TEMP_ADC_QUADRATIC_DIVISOR 131072.0
#define SENSOR_TEMP_T1_QUADRATIC_DIVISOR 8192.0
#define SENSOR_TEMP_FINE_DIVISOR 5120.0
#define SENSOR_HUM_FINE_OFFSET 76800.0
#define SENSOR_HUM_H4_MULTIPLIER 64.0
#define SENSOR_HUM_H5_DIVISOR 16384.0
#define SENSOR_HUM_H2_DIVISOR 65536.0
#define SENSOR_HUM_TEMP_COEFFICIENT_DIVISOR 67108864.0
#define SENSOR_HUM_H1_DIVISOR 524288.0
#define SENSOR_HUM_MIN_PERCENT 0.0
#define SENSOR_HUM_MAX_PERCENT 100.0
#define SENSOR_HUNDREDTHS_PER_UNIT 100.0
#define SENSOR_ROUNDING_OFFSET 0.5

typedef struct {
    uint16_t t1;
    int16_t t2;
    int16_t t3;
    uint8_t h1;
    int16_t h2;
    uint8_t h3;
    int16_t h4;
    int16_t h5;
    int8_t h6;
} sensor_calibration_t;

static I2C_HandleTypeDef* sensor_i2c;
static sensor_calibration_t calibration;

static uint16_t sensor_u16(const uint8_t* bytes)
{
    return (uint16_t)((uint16_t)bytes[0] | ((uint16_t)bytes[1] << SENSOR_BYTE_BITS));
}

static int16_t sensor_signed(uint16_t value, uint16_t sign_bit)
{
    return value & sign_bit ? (int16_t)((int32_t)value - 2 * (int32_t)sign_bit) : (int16_t)value;
}

static HAL_StatusTypeDef sensor_read(I2C_HandleTypeDef* i2c, uint8_t reg, uint8_t* data, uint16_t size)
{
    return HAL_I2C_Mem_Read(i2c, (uint16_t)(TEMPERATURE_SENSOR_I2C_ADDRESS << 1U), reg, I2C_MEMADD_SIZE_8BIT, data, size, SENSOR_I2C_TIMEOUT_MS);
}

static HAL_StatusTypeDef sensor_write(I2C_HandleTypeDef* i2c, uint8_t reg, uint8_t value)
{
    return HAL_I2C_Mem_Write(i2c, (uint16_t)(TEMPERATURE_SENSOR_I2C_ADDRESS << 1U), reg, I2C_MEMADD_SIZE_8BIT, &value, 1U, SENSOR_I2C_TIMEOUT_MS);
}

static temperature_sensor_status_t sensor_error(void)
{
    Error_Handler();
    return TEMPERATURE_SENSOR_ERROR;
}

static int sensor_wait_ready(I2C_HandleTypeDef* i2c)
{
    for (uint32_t elapsed = 0; elapsed <= SENSOR_READY_TIMEOUT_MS; ++elapsed) {
        uint8_t status;
        if (sensor_read(i2c, SENSOR_REG_STATUS, &status, 1U) != HAL_OK) {
            return 0;
        }
        if ((status & SENSOR_STATUS_BUSY) == 0U) {
            return 1;
        }
        if (elapsed < SENSOR_READY_TIMEOUT_MS) {
            HAL_Delay(SENSOR_READY_POLL_INTERVAL_MS);
        }
    }
    return 0;
}

temperature_sensor_status_t temperature_sensor_init(temperature_sensor_handle_t* handle, I2C_HandleTypeDef* i2c)
{
    if (handle == NULL || i2c == NULL) {
        return sensor_error();
    }

    *handle = SENSOR_INVALID_HANDLE;
    sensor_i2c = NULL;

    uint8_t id;
    uint8_t temp_calib[SENSOR_TEMP_CALIB_SIZE];
    uint8_t hum_calib[SENSOR_HUM_CALIB_SIZE];
    sensor_calibration_t new_calibration;
    if (sensor_read(i2c, SENSOR_REG_CHIP_ID, &id, 1U) != HAL_OK || id != SENSOR_CHIP_ID || !sensor_wait_ready(i2c) ||
        sensor_write(i2c, SENSOR_REG_CTRL_MEAS, SENSOR_SLEEP_MODE) != HAL_OK || !sensor_wait_ready(i2c) ||
        sensor_read(i2c, SENSOR_REG_TEMP_CALIB, temp_calib, sizeof(temp_calib)) != HAL_OK || sensor_read(i2c, SENSOR_REG_HUM_CALIB_1, &new_calibration.h1, 1U) != HAL_OK ||
        sensor_read(i2c, SENSOR_REG_HUM_CALIB_2, hum_calib, sizeof(hum_calib)) != HAL_OK) {
        return sensor_error();
    }

    new_calibration.t1 = sensor_u16(temp_calib);
    new_calibration.t2 = sensor_signed(sensor_u16(temp_calib + SENSOR_TEMP_CALIB_T2_OFFSET), SENSOR_SIGN_BIT_16);
    new_calibration.t3 = sensor_signed(sensor_u16(temp_calib + SENSOR_TEMP_CALIB_T3_OFFSET), SENSOR_SIGN_BIT_16);
    new_calibration.h2 = sensor_signed(sensor_u16(hum_calib), SENSOR_SIGN_BIT_16);
    new_calibration.h3 = hum_calib[SENSOR_HUM_CALIB_H3_OFFSET];
    new_calibration.h4 = sensor_signed(
        (uint16_t)(((uint16_t)hum_calib[SENSOR_HUM_CALIB_H4_MSB_OFFSET] << SENSOR_NIBBLE_BITS) | (hum_calib[SENSOR_HUM_CALIB_H4_H5_LSB_OFFSET] & SENSOR_NIBBLE_MASK)),
        SENSOR_SIGN_BIT_12
    );
    new_calibration.h5 = sensor_signed(
        (uint16_t)(((uint16_t)hum_calib[SENSOR_HUM_CALIB_H5_MSB_OFFSET] << SENSOR_NIBBLE_BITS) | (hum_calib[SENSOR_HUM_CALIB_H4_H5_LSB_OFFSET] >> SENSOR_NIBBLE_BITS)),
        SENSOR_SIGN_BIT_12
    );
    new_calibration.h6 = (int8_t)sensor_signed(hum_calib[SENSOR_HUM_CALIB_H6_OFFSET], SENSOR_SIGN_BIT_8);

    if (new_calibration.t1 == 0U || new_calibration.t1 == UINT16_MAX || sensor_write(i2c, SENSOR_REG_CTRL_HUM, SENSOR_HUM_OVERSAMPLING_1) != HAL_OK) {
        return sensor_error();
    }

    calibration = new_calibration;
    sensor_i2c = i2c;
    *handle = SENSOR_HANDLE_VALUE;
    return TEMPERATURE_SENSOR_OK;
}

temperature_sensor_status_t temperature_sensor_get_data(temperature_sensor_handle_t handle, temperature_sensor_data_t* data)
{
    if (data == NULL || handle != SENSOR_HANDLE_VALUE || sensor_i2c == NULL) {
        return sensor_error();
    }

    if (sensor_write(sensor_i2c, SENSOR_REG_CTRL_MEAS, SENSOR_FORCED_TEMP_OVERSAMPLING_1) != HAL_OK) {
        return sensor_error();
    }
    HAL_Delay(SENSOR_MEASUREMENT_TIME_MS);
    uint8_t raw[SENSOR_DATA_SIZE];
    if (!sensor_wait_ready(sensor_i2c) || sensor_read(sensor_i2c, SENSOR_REG_DATA, raw, sizeof(raw)) != HAL_OK) {
        return sensor_error();
    }

    uint32_t adc_t = ((uint32_t)raw[SENSOR_DATA_TEMP_MSB_OFFSET] << SENSOR_TEMP_MSB_SHIFT) | ((uint32_t)raw[SENSOR_DATA_TEMP_LSB_OFFSET] << SENSOR_NIBBLE_BITS) |
                     ((uint32_t)raw[SENSOR_DATA_TEMP_XLSB_OFFSET] >> SENSOR_NIBBLE_BITS);
    uint32_t adc_h = ((uint32_t)raw[SENSOR_DATA_HUM_MSB_OFFSET] << SENSOR_BYTE_BITS) | raw[SENSOR_DATA_HUM_LSB_OFFSET];
    double var1 = ((double)adc_t / SENSOR_TEMP_ADC_LINEAR_DIVISOR - (double)calibration.t1 / SENSOR_TEMP_T1_LINEAR_DIVISOR) * (double)calibration.t2;
    double var2 = (double)adc_t / SENSOR_TEMP_ADC_QUADRATIC_DIVISOR - (double)calibration.t1 / SENSOR_TEMP_T1_QUADRATIC_DIVISOR;
    var2 = var2 * var2 * (double)calibration.t3;
    double temperature = (var1 + var2) / SENSOR_TEMP_FINE_DIVISOR;
    double scaled_temperature = temperature * SENSOR_HUNDREDTHS_PER_UNIT;
    if (scaled_temperature < INT16_MIN || scaled_temperature > INT16_MAX) {
        return sensor_error();
    }
    int32_t t_fine = (int32_t)(var1 + var2);
    double humidity = (double)t_fine - SENSOR_HUM_FINE_OFFSET;
    humidity = ((double)adc_h - ((double)calibration.h4 * SENSOR_HUM_H4_MULTIPLIER + (double)calibration.h5 / SENSOR_HUM_H5_DIVISOR * humidity)) *
               ((double)calibration.h2 / SENSOR_HUM_H2_DIVISOR *
                (1.0 + (double)calibration.h6 / SENSOR_HUM_TEMP_COEFFICIENT_DIVISOR * humidity * (1.0 + (double)calibration.h3 / SENSOR_HUM_TEMP_COEFFICIENT_DIVISOR * humidity)));
    humidity *= 1.0 - (double)calibration.h1 * humidity / SENSOR_HUM_H1_DIVISOR;
    if (humidity < SENSOR_HUM_MIN_PERCENT) {
        humidity = SENSOR_HUM_MIN_PERCENT;
    }
    else if (humidity > SENSOR_HUM_MAX_PERCENT) {
        humidity = SENSOR_HUM_MAX_PERCENT;
    }

    data->temperature = (int16_t)(scaled_temperature + (scaled_temperature < 0.0 ? -SENSOR_ROUNDING_OFFSET : SENSOR_ROUNDING_OFFSET));
    data->humidity = (int16_t)(humidity * SENSOR_HUNDREDTHS_PER_UNIT + SENSOR_ROUNDING_OFFSET);
    return TEMPERATURE_SENSOR_OK;
}