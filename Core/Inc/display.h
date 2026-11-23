#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

#include "real_time_clock.h"
#include "stm32f4xx_hal.h"

#define DISPLAY_WIDTH 128U
#define DISPLAY_HEIGHT 64U

typedef int display_handle_t;

typedef enum {
  DISPLAY_OK = 0,
  DISPLAY_ERROR_ARGUMENT,
  DISPLAY_ERROR_NOT_INITIALIZED,
  DISPLAY_ERROR_IO
} display_status_t;

display_status_t display_init(display_handle_t* handle, I2C_HandleTypeDef* i2c);
display_status_t display_show_date(display_handle_t handle, rtc_date_t date);

#endif
