#include "display.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "calendar.h"
#include "font.h"
#include "ssd1306.h"

#define DISPLAY_ADDRESS 0x3C
#define DISPLAY_I2C_TIMEOUT_MS 100
#define DISPLAY_HANDLE_VALUE 1
#define DISPLAY_PAGE_HEIGHT 8U
#define DISPLAY_FONT_HEIGHT 8U
#define DISPLAY_FIRST_CHARACTER ' '
#define DISPLAY_LAST_CHARACTER '~'
#define DISPLAY_TIME_TEXT_LENGTH 8
#define DISPLAY_DATE_TEXT_LENGTH 14
#define DISPLAY_CLIMATE_TEXT_CAPACITY 17U
#define DISPLAY_HUNDREDTHS_PER_UNIT 100
#define DISPLAY_MAX_HUMIDITY_HUNDREDTHS 10000
#define DISPLAY_CLIMATE_LINE_Y (2U * DISPLAY_FONT_HEIGHT)
#define DISPLAY_MAX_HOURS 23U
#define DISPLAY_MAX_MINUTES 59U
#define DISPLAY_MAX_SECONDS 59U

static display_handle_t display_handle = DISPLAY_HANDLE_VALUE;

static uint8_t framebuffer[DISPLAY_WIDTH * (DISPLAY_HEIGHT / DISPLAY_PAGE_HEIGHT)];
static OLED_Config oled_config;

static int32_t display_send_i2c(void* context, uint8_t address, const uint8_t* data, size_t length);
display_status_t display_clear(void);
display_status_t display_draw_text(uint16_t x, uint16_t y, const char* text);
display_status_t display_flush(void);
bool is_char_invalid(const char* currentChar);
bool is_position_invalid(uint16_t x, uint16_t y);
display_status_t display_draw_char(uint16_t x, uint16_t y, char c);

display_status_t display_init(display_handle_t* handle, I2C_HandleTypeDef* i2c)
{
    if (*handle == display_handle) {
        return DISPLAY_OK;
    }

    if (i2c == NULL) {
        return DISPLAY_ERROR_ARGUMENT;
    }

    oled_config.bus_type = OLED_BUS_I2C;
    oled_config.width = DISPLAY_WIDTH;
    oled_config.height = DISPLAY_HEIGHT;
    oled_config.user_context = i2c;
    oled_config.transport.i2c.i2c_address_7bit = DISPLAY_ADDRESS;
    oled_config.transport.i2c.send_fn = display_send_i2c;

    if (OLED_Init(&oled_config) != OLED_OK) {
        return DISPLAY_ERROR_IO;
    }

    memset(framebuffer, 0, sizeof(framebuffer));

    *handle = display_handle;
    return DISPLAY_OK;
}

static display_status_t display_format_time(rtc_date_t date, char* line, size_t size)
{
    if (date.hours > DISPLAY_MAX_HOURS || date.minutes > DISPLAY_MAX_MINUTES || date.seconds > DISPLAY_MAX_SECONDS) {
        return DISPLAY_ERROR_ARGUMENT;
    }

    int length = snprintf(line, size, "%02u:%02u:%02u", (unsigned)date.hours, (unsigned)date.minutes, (unsigned)date.seconds);
    return length == DISPLAY_TIME_TEXT_LENGTH && (size_t)length < size ? DISPLAY_OK : DISPLAY_ERROR_ARGUMENT;
}

static display_status_t display_format_date(rtc_date_t date, char* line, size_t size)
{
    static const char* const weekdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

    uint8_t weekday = calendar_weekday(date.year, date.month, date.day);
    if (weekday == 0U) {
        return DISPLAY_ERROR_ARGUMENT;
    }

    int length = snprintf(line, size, "%s %02u.%02u.%04u", weekdays[weekday - 1U], (unsigned)date.day, (unsigned)date.month, (unsigned)date.year);
    return length == DISPLAY_DATE_TEXT_LENGTH && (size_t)length < size ? DISPLAY_OK : DISPLAY_ERROR_ARGUMENT;
}

static display_status_t display_format_climate(const temperature_sensor_data_t* reading, char* line, size_t size)
{
    if (reading->humidity < 0 || reading->humidity > DISPLAY_MAX_HUMIDITY_HUNDREDTHS) {
        return DISPLAY_ERROR_ARGUMENT;
    }

    int32_t temperature = reading->temperature;
    uint32_t magnitude = (uint32_t)(temperature < 0 ? -temperature : temperature);
    unsigned humidity = (unsigned)reading->humidity;
    int length = snprintf(
        line,
        size,
        "%s%lu.%02luC %u.%02u%%",
        temperature < 0 ? "-" : "",
        (unsigned long)(magnitude / DISPLAY_HUNDREDTHS_PER_UNIT),
        (unsigned long)(magnitude % DISPLAY_HUNDREDTHS_PER_UNIT),
        humidity / DISPLAY_HUNDREDTHS_PER_UNIT,
        humidity % DISPLAY_HUNDREDTHS_PER_UNIT
    );
    return length >= 0 && (size_t)length < size ? DISPLAY_OK : DISPLAY_ERROR_ARGUMENT;
}

display_status_t display_show_data(display_handle_t handle, rtc_date_t date, temperature_sensor_data_t reading)
{
    if (handle != display_handle) {
        return DISPLAY_ERROR_ARGUMENT;
    }

    char time_line[DISPLAY_TIME_TEXT_LENGTH + 1];
    char date_line[DISPLAY_DATE_TEXT_LENGTH + 1];
    char climate_line[DISPLAY_CLIMATE_TEXT_CAPACITY];

    if (display_format_time(date, time_line, sizeof(time_line)) != DISPLAY_OK || display_format_date(date, date_line, sizeof(date_line)) != DISPLAY_OK ||
        display_format_climate(&reading, climate_line, sizeof(climate_line)) != DISPLAY_OK) {
        return DISPLAY_ERROR_ARGUMENT;
    }

    display_status_t status = display_clear();
    if (status != DISPLAY_OK) {
        return status;
    }
    status = display_draw_text(0U, 0U, time_line);
    if (status != DISPLAY_OK) {
        return status;
    }
    status = display_draw_text(0U, DISPLAY_FONT_HEIGHT, date_line);
    if (status != DISPLAY_OK) {
        return status;
    }
    status = display_draw_text(0U, DISPLAY_CLIMATE_LINE_Y, climate_line);
    if (status != DISPLAY_OK) {
        return status;
    }
    return display_flush();
}

static int32_t display_send_i2c(void* context, uint8_t address, const uint8_t* data, size_t length)
{
    if (length > UINT16_MAX) {
        return OLED_ERR_INVALID_ARG;
    }
    return HAL_I2C_Master_Transmit((I2C_HandleTypeDef*)context, (uint16_t)(address << 1), (uint8_t*)data, (uint16_t)length, DISPLAY_I2C_TIMEOUT_MS) == HAL_OK ? OLED_OK
                                                                                                                                                              : OLED_ERR_IO;
}

display_status_t display_clear(void)
{
    memset(framebuffer, 0, sizeof(framebuffer));
    return DISPLAY_OK;
}

display_status_t display_draw_text(uint16_t x, uint16_t y, const char* text)
{
    if (text == NULL || is_position_invalid(x, y)) {
        return DISPLAY_ERROR_ARGUMENT;
    }

    uint32_t charIndex = 0;
    do {
        const char* currentChar = text + charIndex;

        if (is_char_invalid(currentChar)) {
            return DISPLAY_ERROR_ARGUMENT;
        }

        uint16_t pixel_x = x + charIndex * FONT_ADVANCE;
        if (is_position_invalid(pixel_x, y)) {
            return DISPLAY_ERROR_ARGUMENT;
        }

        if (display_draw_char(pixel_x, y, *currentChar) != DISPLAY_OK) {
            return DISPLAY_ERROR_IO;
        }

        charIndex++;
    } while (text[charIndex] != '\0');

    return DISPLAY_OK;
}

bool is_char_invalid(const char* currentChar)
{
    return (*currentChar < DISPLAY_FIRST_CHARACTER || *currentChar > DISPLAY_LAST_CHARACTER);
}

bool is_position_invalid(uint16_t x, uint16_t y)
{
    return (x >= DISPLAY_WIDTH - FONT_ADVANCE || y > DISPLAY_HEIGHT - DISPLAY_FONT_HEIGHT);
}

display_status_t display_draw_char(uint16_t x, uint16_t y, char c)
{
    for (uint16_t col = 0; col < FONT_ADVANCE; ++col) {
        uint8_t bits = col < FONT_WIDTH ? font[c - DISPLAY_FIRST_CHARACTER][col] : 0;

        for (uint16_t row = 0U; row < DISPLAY_FONT_HEIGHT; ++row) {
            uint16_t pixel_y = y + row;
            size_t index = (size_t)(pixel_y / DISPLAY_PAGE_HEIGHT) * DISPLAY_WIDTH + x + col;
            uint8_t mask = (uint8_t)(1U << (pixel_y % DISPLAY_PAGE_HEIGHT));

            if (bits & (1U << row)) {
                framebuffer[index] |= mask;
            }
            else {
                framebuffer[index] &= (uint8_t)~mask;
            }
        }
    }

    return DISPLAY_OK;
}

display_status_t display_flush(void)
{
    return OLED_DrawBitmap(&oled_config, framebuffer, sizeof(framebuffer)) == OLED_OK ? DISPLAY_OK : DISPLAY_ERROR_IO;
}
