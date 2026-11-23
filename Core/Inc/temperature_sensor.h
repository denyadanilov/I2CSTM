#ifndef TEMPERATURE_SENSOR_H
#define TEMPERATURE_SENSOR_H

#include <stdint.h>

#include "stm32f4xx_hal.h"

typedef int temperature_sensor_handle_t;

typedef enum {
  TEMPERATURE_SENSOR_OK = 0,
  TEMPERATURE_SENSOR_ERROR = 1
} temperature_sensor_status_t;

typedef struct {
  /* Hundredths of a degree Celsius and hundredths of percent RH. */
  int16_t temperature;
  int16_t humidity;
} temperature_sensor_data_t;

temperature_sensor_status_t temperature_sensor_init(
    temperature_sensor_handle_t* handle, I2C_HandleTypeDef* i2c);
temperature_sensor_status_t temperature_sensor_get_data(
    temperature_sensor_handle_t handle, temperature_sensor_data_t* data);

#endif  // TEMPERATURE_SENSOR_H