#ifndef RTC_BUILD_TIME_H
#define RTC_BUILD_TIME_H

#include <stdbool.h>
#include <stdint.h>

/* Writes seven DS1307 time registers; weekday uses Sunday = 1. */
bool rtc_get_build_time(uint8_t* registers);

#endif  // RTC_BUILD_TIME_H
