#ifndef REAL_TIME_CLOCK_H
#define REAL_TIME_CLOCK_H

#include <stdint.h>

#include "stm32f4xx_hal.h"

typedef int rtc_handle_t;

typedef enum { RTC_OK = 0, RTC_ERROR = 1 } rtc_status_t;

typedef struct {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
    uint8_t day;
    uint8_t month;
    uint16_t year;
} rtc_date_t;

rtc_status_t rtc_init(rtc_handle_t* handle, I2C_HandleTypeDef* i2c);
rtc_status_t rtc_get_date(rtc_handle_t handle, rtc_date_t* date);

#endif  // REAL_TIME_CLOCK_H