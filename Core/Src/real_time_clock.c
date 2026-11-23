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
#define RTC_INVALID_HANDLE 0
#define RTC_SECONDS_OFFSET 0U
#define RTC_MINUTES_OFFSET 1U
#define RTC_HOURS_OFFSET 2U
#define RTC_DAY_OFFSET 4U
#define RTC_MONTH_OFFSET 5U
#define RTC_YEAR_OFFSET 6U
#define RTC_SECONDS_CH_BIT 0x80U
#define RTC_SECONDS_MASK 0x7FU
#define RTC_MINUTES_MASK 0x7FU
#define RTC_HOURS_12_HOUR_BIT 0x40U
#define RTC_HOURS_PM_BIT 0x20U
#define RTC_HOURS_12_HOUR_MASK 0x1FU
#define RTC_HOURS_24_HOUR_MASK 0x3FU
#define RTC_MONTH_MASK 0x1FU
#define RTC_BCD_DIGIT_MASK 0x0FU
#define RTC_BCD_TENS_SHIFT 4U
#define RTC_BCD_MAX_DIGIT 9U
#define RTC_DECIMAL_BASE 10U
#define RTC_MIN_12_HOUR 1U
#define RTC_HOURS_PER_HALF_DAY 12U
#define RTC_MAX_SECONDS 59U
#define RTC_MAX_MINUTES 59U
#define RTC_MAX_HOURS 23U
#define RTC_MIN_DAY 1U
#define RTC_MAX_DAY 31U
#define RTC_MIN_MONTH 1U
#define RTC_MAX_MONTH 12U
#define RTC_YEAR_BASE 2000U

static I2C_HandleTypeDef* rtc_i2c;

static bool rtc_bcd_to_decimal(uint8_t bcd, uint8_t* value)
{
    uint8_t ones = (uint8_t)(bcd & RTC_BCD_DIGIT_MASK);
    uint8_t tens = (uint8_t)((bcd >> RTC_BCD_TENS_SHIFT) & RTC_BCD_DIGIT_MASK);

    if (ones > RTC_BCD_MAX_DIGIT || tens > RTC_BCD_MAX_DIGIT) {
        return false;
    }

    *value = (uint8_t)(tens * RTC_DECIMAL_BASE + ones);
    return true;
}

static void rtc_handle_error(void)
{
    Error_Handler();
}

rtc_status_t rtc_init(rtc_handle_t* handle, I2C_HandleTypeDef* i2c)
{
    if (handle == NULL || i2c == NULL) {
        rtc_handle_error();
        return RTC_ERROR;
    }

    *handle = RTC_INVALID_HANDLE;
    rtc_i2c = NULL;

    uint8_t seconds;

    if (HAL_I2C_Mem_Read(i2c, (uint16_t)(RTC_I2C_ADDRESS << 1), RTC_REGISTER_SECONDS, I2C_MEMADD_SIZE_8BIT, &seconds, 1, RTC_I2C_TIMEOUT_MS) != HAL_OK) {
        rtc_handle_error();
        return RTC_ERROR;
    }

    if ((seconds & RTC_SECONDS_CH_BIT) != 0U) {
        uint8_t build_time[RTC_TIME_REGISTER_COUNT];

        if (!rtc_get_build_time(build_time)) {
            rtc_handle_error();
            return RTC_ERROR;
        }

        if (HAL_I2C_Mem_Write(i2c, (uint16_t)(RTC_I2C_ADDRESS << 1U), RTC_REGISTER_TIME, I2C_MEMADD_SIZE_8BIT, build_time, RTC_TIME_REGISTER_COUNT, RTC_I2C_TIMEOUT_MS) != HAL_OK) {
            rtc_handle_error();
            return RTC_ERROR;
        }
    }

    rtc_i2c = i2c;

    *handle = RTC_HANDLE_VALUE;
    return RTC_OK;
}

rtc_status_t rtc_get_date(rtc_handle_t handle, rtc_date_t* date)
{
    if (date == NULL) {
        rtc_handle_error();
        return RTC_ERROR;
    }

    if (handle != RTC_HANDLE_VALUE || rtc_i2c == NULL) {
        rtc_handle_error();
        return RTC_ERROR;
    }

    uint8_t registers[RTC_TIME_REGISTER_COUNT];
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day;
    uint8_t month;
    uint8_t year;

    if (HAL_I2C_Mem_Read(rtc_i2c, (uint16_t)(RTC_I2C_ADDRESS << 1U), RTC_REGISTER_TIME, I2C_MEMADD_SIZE_8BIT, registers, RTC_TIME_REGISTER_COUNT, RTC_I2C_TIMEOUT_MS) != HAL_OK) {
        rtc_handle_error();
        return RTC_ERROR;
    }

    if (!rtc_bcd_to_decimal((uint8_t)(registers[RTC_SECONDS_OFFSET] & RTC_SECONDS_MASK), &seconds) ||
        !rtc_bcd_to_decimal((uint8_t)(registers[RTC_MINUTES_OFFSET] & RTC_MINUTES_MASK), &minutes)) {
        rtc_handle_error();
        return RTC_ERROR;
    }

    if ((registers[RTC_HOURS_OFFSET] & RTC_HOURS_12_HOUR_BIT) != 0U) {
        if (!rtc_bcd_to_decimal((uint8_t)(registers[RTC_HOURS_OFFSET] & RTC_HOURS_12_HOUR_MASK), &hours) || hours < RTC_MIN_12_HOUR || hours > RTC_HOURS_PER_HALF_DAY) {
            rtc_handle_error();
            return RTC_ERROR;
        }
        if ((registers[RTC_HOURS_OFFSET] & RTC_HOURS_PM_BIT) != 0U) {
            hours = (uint8_t)(hours == RTC_HOURS_PER_HALF_DAY ? RTC_HOURS_PER_HALF_DAY : hours + RTC_HOURS_PER_HALF_DAY);
        }
        else if (hours == RTC_HOURS_PER_HALF_DAY) {
            hours = 0U;
        }
    }
    else if (!rtc_bcd_to_decimal((uint8_t)(registers[RTC_HOURS_OFFSET] & RTC_HOURS_24_HOUR_MASK), &hours)) {
        rtc_handle_error();
        return RTC_ERROR;
    }

    if (!rtc_bcd_to_decimal(registers[RTC_DAY_OFFSET], &day) || !rtc_bcd_to_decimal((uint8_t)(registers[RTC_MONTH_OFFSET] & RTC_MONTH_MASK), &month) ||
        !rtc_bcd_to_decimal(registers[RTC_YEAR_OFFSET], &year) || seconds > RTC_MAX_SECONDS || minutes > RTC_MAX_MINUTES || hours > RTC_MAX_HOURS || day < RTC_MIN_DAY ||
        day > RTC_MAX_DAY || month < RTC_MIN_MONTH || month > RTC_MAX_MONTH) {
        rtc_handle_error();
        return RTC_ERROR;
    }

    date->seconds = seconds;
    date->minutes = minutes;
    date->hours = hours;
    date->day = day;
    date->month = month;
    date->year = (uint16_t)(RTC_YEAR_BASE + year);

    return RTC_OK;
}