#include "real_time_clock.h"

#include <stdbool.h>
#include <stddef.h>

#include "main.h"
#include "rtc_build_time.h"

#define RTC_I2C_ADDRESS 0x68U
#define RTC_I2C_TIMEOUT_MS 100U
#define RTC_REGISTER_SECONDS 0x00U
#define RTC_REGISTER_TIME 0x00U
#define RTC_TIME_REGISTER_COUNT 7U
#define RTC_HANDLE_VALUE 1
#define RTC_SECONDS_CH_BIT 0x80U
#define RTC_HOURS_12_HOUR_BIT 0x40U
#define RTC_HOURS_PM_BIT 0x20U

static I2C_HandleTypeDef* rtc_i2c;

static bool rtc_bcd_to_decimal(uint8_t bcd, uint8_t* value) {
  uint8_t ones = (uint8_t)(bcd & 0x0FU);
  uint8_t tens = (uint8_t)((bcd >> 4U) & 0x0FU);

  if (ones > 9U || tens > 9U) {
    return false;
  }

  *value = (uint8_t)(tens * 10U + ones);
  return true;
}

static void rtc_handle_error(void) { Error_Handler(); }

void rtc_init(rtc_handle_t* handle, I2C_HandleTypeDef* i2c) {
  if (handle == NULL || i2c == NULL) {
    rtc_handle_error();
    return;
  }

  *handle = 0;
  rtc_i2c = NULL;

  uint8_t seconds;

  if (HAL_I2C_Mem_Read(i2c, (uint16_t)(RTC_I2C_ADDRESS << 1),
                       RTC_REGISTER_SECONDS, I2C_MEMADD_SIZE_8BIT, &seconds, 1,
                       RTC_I2C_TIMEOUT_MS) != HAL_OK) {
    rtc_handle_error();
    return;
  }

  if ((seconds & RTC_SECONDS_CH_BIT) != 0U) {
    uint8_t build_time[RTC_TIME_REGISTER_COUNT];

    if (!rtc_get_build_time(build_time)) {
      rtc_handle_error();
      return;
    }

    if (HAL_I2C_Mem_Write(i2c, (uint16_t)(RTC_I2C_ADDRESS << 1U),
                          RTC_REGISTER_TIME, I2C_MEMADD_SIZE_8BIT, build_time,
                          RTC_TIME_REGISTER_COUNT,
                          RTC_I2C_TIMEOUT_MS) != HAL_OK) {
      rtc_handle_error();
      return;
    }
  }

  rtc_i2c = i2c;

  *handle = RTC_HANDLE_VALUE;
}

rtc_date_t rtc_get_date(rtc_handle_t handle) {
  rtc_date_t date = {0};

  if (handle != RTC_HANDLE_VALUE || rtc_i2c == NULL) {
    rtc_handle_error();
    return date;
  }

  uint8_t registers[RTC_TIME_REGISTER_COUNT];
  uint8_t seconds;
  uint8_t minutes;
  uint8_t hours;
  uint8_t day;
  uint8_t month;
  uint8_t year;

  if (HAL_I2C_Mem_Read(rtc_i2c, (uint16_t)(RTC_I2C_ADDRESS << 1U),
                       RTC_REGISTER_TIME, I2C_MEMADD_SIZE_8BIT, registers,
                       RTC_TIME_REGISTER_COUNT, RTC_I2C_TIMEOUT_MS) != HAL_OK) {
    rtc_handle_error();
    return date;
  }

  if (!rtc_bcd_to_decimal((uint8_t)(registers[0] & 0x7FU), &seconds) ||
      !rtc_bcd_to_decimal((uint8_t)(registers[1] & 0x7FU), &minutes)) {
    rtc_handle_error();
    return date;
  }

  if ((registers[2] & RTC_HOURS_12_HOUR_BIT) != 0U) {
    if (!rtc_bcd_to_decimal((uint8_t)(registers[2] & 0x1FU), &hours) ||
        hours < 1U || hours > 12U) {
      rtc_handle_error();
      return date;
    }
    if ((registers[2] & RTC_HOURS_PM_BIT) != 0U) {
      hours = (uint8_t)(hours == 12U ? 12U : hours + 12U);
    } else if (hours == 12U) {
      hours = 0U;
    }
  } else if (!rtc_bcd_to_decimal((uint8_t)(registers[2] & 0x3FU), &hours)) {
    rtc_handle_error();
    return date;
  }

  if (!rtc_bcd_to_decimal(registers[4], &day) ||
      !rtc_bcd_to_decimal((uint8_t)(registers[5] & 0x1FU), &month) ||
      !rtc_bcd_to_decimal(registers[6], &year) || seconds > 59U ||
      minutes > 59U || hours > 23U || day < 1U || day > 31U || month < 1U ||
      month > 12U) {
    rtc_handle_error();
    return date;
  }

  date.seconds = seconds;
  date.minutes = minutes;
  date.hours = hours;
  date.day = day;
  date.month = month;
  date.year = (uint16_t)(2000U + year);
  return date;
}