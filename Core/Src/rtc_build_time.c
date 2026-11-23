#include "rtc_build_time.h"

#include <string.h>

#include "calendar.h"

#define RTC_BUILD_DATE __DATE__
#define RTC_BUILD_TIME __TIME__
#define RTC_DATE_MONTH_NAME_LENGTH 3U
#define RTC_DATE_DAY_OFFSET 4U
#define RTC_DATE_YEAR_OFFSET 7U
#define RTC_DATE_YEAR_DIGITS 4U
#define RTC_TIME_MINUTE_OFFSET 3U
#define RTC_TIME_SECOND_OFFSET 6U
#define RTC_MONTH_COUNT 12U
#define RTC_DECIMAL_BASE 10U
#define RTC_BCD_TENS_SHIFT 4U
#define RTC_MAX_HOURS 23U
#define RTC_MAX_MINUTES 59U
#define RTC_MAX_SECONDS 59U
#define RTC_MIN_DAY 1U
#define RTC_MAX_DAY 31U
#define RTC_MIN_YEAR 2000U
#define RTC_MAX_YEAR 2099U
#define RTC_YEARS_PER_CENTURY 100U
#define RTC_SECONDS_OFFSET 0U
#define RTC_MINUTES_OFFSET 1U
#define RTC_HOURS_OFFSET 2U
#define RTC_WEEKDAY_OFFSET 3U
#define RTC_DAY_OFFSET 4U
#define RTC_MONTH_OFFSET 5U
#define RTC_YEAR_OFFSET 6U

static bool rtc_parse_digit(char character, uint8_t* digit)
{
    if (character < '0' || character > '9') {
        return false;
    }
    *digit = (uint8_t)(character - '0');
    return true;
}

static bool rtc_parse_pair(const char* text, uint8_t* value)
{
    uint8_t tens;
    uint8_t ones;
    if (!rtc_parse_digit(text[0], &tens) || !rtc_parse_digit(text[1], &ones)) {
        return false;
    }
    *value = (uint8_t)(tens * RTC_DECIMAL_BASE + ones);
    return true;
}

static uint8_t rtc_decimal_to_bcd(uint8_t value)
{
    return (uint8_t)(((value / RTC_DECIMAL_BASE) << RTC_BCD_TENS_SHIFT) | (value % RTC_DECIMAL_BASE));
}

bool rtc_get_build_time(uint8_t* registers)
{
    static const char* const month_names[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    uint8_t month = 0U;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint16_t year = 0U;

    for (uint8_t i = 0U; i < RTC_MONTH_COUNT; ++i) {
        if (strncmp(RTC_BUILD_DATE, month_names[i], RTC_DATE_MONTH_NAME_LENGTH) == 0) {
            month = (uint8_t)(i + 1U);
            break;
        }
    }
    if (month == 0U) {
        return false;
    }

    const char* build_day = &RTC_BUILD_DATE[RTC_DATE_DAY_OFFSET];
    if (build_day[0] == ' ') {
        if (!rtc_parse_digit(build_day[1], &day)) {
            return false;
        }
    }
    else if (!rtc_parse_pair(build_day, &day)) {
        return false;
    }

    for (uint8_t i = 0U; i < RTC_DATE_YEAR_DIGITS; ++i) {
        uint8_t digit;
        if (!rtc_parse_digit(RTC_BUILD_DATE[RTC_DATE_YEAR_OFFSET + i], &digit)) {
            return false;
        }
        year = (uint16_t)(year * RTC_DECIMAL_BASE + digit);
    }

    if (!rtc_parse_pair(RTC_BUILD_TIME, &hour) || !rtc_parse_pair(&RTC_BUILD_TIME[RTC_TIME_MINUTE_OFFSET], &minute) ||
        !rtc_parse_pair(&RTC_BUILD_TIME[RTC_TIME_SECOND_OFFSET], &second) || hour > RTC_MAX_HOURS || minute > RTC_MAX_MINUTES || second > RTC_MAX_SECONDS || day < RTC_MIN_DAY ||
        day > RTC_MAX_DAY || year < RTC_MIN_YEAR || year > RTC_MAX_YEAR) {
        return false;
    }

    uint8_t weekday = calendar_weekday(year, month, day);
    if (weekday == 0U) {
        return false;
    }

    registers[RTC_SECONDS_OFFSET] = rtc_decimal_to_bcd(second);
    registers[RTC_MINUTES_OFFSET] = rtc_decimal_to_bcd(minute);
    registers[RTC_HOURS_OFFSET] = rtc_decimal_to_bcd(hour);
    registers[RTC_WEEKDAY_OFFSET] = weekday;
    registers[RTC_DAY_OFFSET] = rtc_decimal_to_bcd(day);
    registers[RTC_MONTH_OFFSET] = rtc_decimal_to_bcd(month);
    registers[RTC_YEAR_OFFSET] = rtc_decimal_to_bcd((uint8_t)(year % RTC_YEARS_PER_CENTURY));
    return true;
}
