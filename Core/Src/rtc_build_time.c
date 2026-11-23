#include "rtc_build_time.h"

#include <string.h>

#include "calendar.h"

#define RTC_BUILD_DATE __DATE__
#define RTC_BUILD_TIME __TIME__

static bool rtc_parse_digit(char character, uint8_t* digit) {
  if (character < '0' || character > '9') {
    return false;
  }
  *digit = (uint8_t)(character - '0');
  return true;
}

static bool rtc_parse_pair(const char* text, uint8_t* value) {
  uint8_t tens;
  uint8_t ones;
  if (!rtc_parse_digit(text[0], &tens) || !rtc_parse_digit(text[1], &ones)) {
    return false;
  }
  *value = (uint8_t)(tens * 10U + ones);
  return true;
}

static uint8_t rtc_decimal_to_bcd(uint8_t value) {
  return (uint8_t)(((value / 10U) << 4U) | (value % 10U));
}

bool rtc_get_build_time(uint8_t* registers) {
  static const char* const month_names[] = {"Jan", "Feb", "Mar", "Apr",
                                            "May", "Jun", "Jul", "Aug",
                                            "Sep", "Oct", "Nov", "Dec"};
  uint8_t month = 0U;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  uint8_t year_digits[4];

  for (uint8_t i = 0U; i < 12U; ++i) {
    if (strncmp(RTC_BUILD_DATE, month_names[i], 3U) == 0) {
      month = (uint8_t)(i + 1U);
      break;
    }
  }
  if (month == 0U) {
    return false;
  }

  const char* build_day = &RTC_BUILD_DATE[4];
  if (build_day[0] == ' ') {
    if (!rtc_parse_digit(build_day[1], &day)) {
      return false;
    }
  } else if (!rtc_parse_pair(build_day, &day)) {
    return false;
  }

  for (uint8_t i = 0U; i < 4U; ++i) {
    if (!rtc_parse_digit(RTC_BUILD_DATE[7U + i], &year_digits[i])) {
      return false;
    }
  }
  uint16_t year = (uint16_t)(year_digits[0] * 1000U + year_digits[1] * 100U +
                             year_digits[2] * 10U + year_digits[3]);

  if (!rtc_parse_pair(RTC_BUILD_TIME, &hour) ||
      !rtc_parse_pair(&RTC_BUILD_TIME[3], &minute) ||
      !rtc_parse_pair(&RTC_BUILD_TIME[6], &second) || hour > 23U ||
      minute > 59U || second > 59U || day < 1U || day > 31U || year < 2000U ||
      year > 2099U) {
    return false;
  }

  uint8_t weekday = calendar_weekday(year, month, day);
  if (weekday == 0U) {
    return false;
  }

  registers[0] = rtc_decimal_to_bcd(second);
  registers[1] = rtc_decimal_to_bcd(minute);
  registers[2] = rtc_decimal_to_bcd(hour);
  registers[3] = weekday;
  registers[4] = rtc_decimal_to_bcd(day);
  registers[5] = rtc_decimal_to_bcd(month);
  registers[6] = rtc_decimal_to_bcd((uint8_t)(year % 100U));
  return true;
}
