#include "calendar.h"

uint8_t calendar_weekday(uint16_t year, uint8_t month, uint8_t day)
{
    static const uint8_t days_per_month[] = {31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U};
    if (year < 1U || year > 9999U || month < 1U || month > 12U) {
        return 0U;
    }

    uint8_t max_day = days_per_month[month - 1U];
    if (month == 2U && year % 4U == 0U && (year % 100U != 0U || year % 400U == 0U)) {
        ++max_day;
    }
    if (day < 1U || day > max_day) {
        return 0U;
    }

    int adjusted_year = year;
    int adjusted_month = month;
    if (adjusted_month < 3) {
        adjusted_month += 12;
        adjusted_year--;
    }

    int century = adjusted_year / 100;
    int year_of_century = adjusted_year % 100;
    int zeller_day = (day + (13 * (adjusted_month + 1)) / 5 + year_of_century + year_of_century / 4 + century / 4 + 5 * century) % 7;
    return (uint8_t)((zeller_day + 6) % 7 + 1);
}
