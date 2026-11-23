#ifndef CALENDAR_H
#define CALENDAR_H

#include <stdint.h>

/* Returns 1 for Sunday through 7 for Saturday, or 0 for an invalid date. */
uint8_t calendar_weekday(uint16_t year, uint8_t month, uint8_t day);

#endif  // CALENDAR_H
