#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "climate_station.h"
#include "display.h"
#include "font.h"
#include "i2c.h"
#include "ssd1306.h"

I2C_HandleTypeDef hi2c1;
static uint8_t screen[DISPLAY_WIDTH * DISPLAY_HEIGHT / 8U];
static unsigned flush_count;
static unsigned sensor_init_count;
static unsigned sensor_read_count;
static unsigned i2c_init_count;
static temperature_sensor_data_t next_reading = {2508, 5429};
static const rtc_date_t test_date = {12, 34, 56, 14, 2, 2026};

void Error_Handler(void)
{
    assert(0 && "unexpected error");
}

void MX_I2C1_Init(void)
{
    ++i2c_init_count;
}

rtc_status_t rtc_init(rtc_handle_t* handle, I2C_HandleTypeDef* i2c)
{
    assert(i2c == &hi2c1);
    *handle = 1;
    return RTC_OK;
}

rtc_status_t rtc_get_date(rtc_handle_t handle, rtc_date_t* date)
{
    assert(handle == 1);
    *date = test_date;
    return RTC_OK;
}

temperature_sensor_status_t temperature_sensor_init(temperature_sensor_handle_t* handle, I2C_HandleTypeDef* i2c)
{
    assert(i2c == &hi2c1);
    ++sensor_init_count;
    *handle = 1;
    return TEMPERATURE_SENSOR_OK;
}

temperature_sensor_status_t temperature_sensor_get_data(temperature_sensor_handle_t handle, temperature_sensor_data_t* reading)
{
    assert(handle == 1);
    ++sensor_read_count;
    *reading = next_reading;
    return TEMPERATURE_SENSOR_OK;
}

HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef* i2c, uint16_t address, uint8_t* data, uint16_t size, uint32_t timeout)
{
    (void)i2c;
    (void)address;
    (void)data;
    (void)size;
    (void)timeout;
    return HAL_OK;
}

int32_t OLED_Init(const OLED_Config* config)
{
    assert(config->user_context == &hi2c1);
    return OLED_OK;
}

int32_t OLED_DrawBitmap(const OLED_Config* config, const uint8_t* bitmap, size_t length)
{
    assert(config->user_context == &hi2c1 && length == sizeof(screen));
    memcpy(screen, bitmap, length);
    ++flush_count;
    return OLED_OK;
}

static void assert_line(unsigned page, const char* text)
{
    const uint8_t* line = screen + page * DISPLAY_WIDTH;
    size_t position = 0;
    for (const char* c = text; *c != '\0'; ++c) {
        for (unsigned col = 0; col < FONT_WIDTH; ++col) {
            assert(line[position++] == font[*c - ' '][col]);
        }
        assert(line[position++] == 0);
    }
    while (position < DISPLAY_WIDTH) {
        assert(line[position++] == 0);
    }
}

int main(void)
{
    climate_station_handle_t station = 0;
    climate_station_init(&station);
    climate_station_init(&station);
    assert(i2c_init_count == 1 && sensor_init_count == 1);
    climate_station_update();
    assert(sensor_read_count == 1 && flush_count == 1);
    assert_line(0, "12:34:56");
    assert_line(1, "Sat 14.02.2026");
    assert_line(2, "25.08C 54.29%");

    next_reading.temperature = -50;
    next_reading.humidity = 10000;
    climate_station_update();
    assert(sensor_read_count == 2 && flush_count == 2);
    assert_line(2, "-0.50C 100.00%");

    next_reading.temperature = INT16_MIN;
    next_reading.humidity = 10000;
    climate_station_update();
    assert_line(2, "-327.68C 100.00%");

    display_handle_t display = 1;
    next_reading.temperature = 0;
    next_reading.humidity = 0;
    assert(display_show_data(display, test_date, next_reading) == DISPLAY_OK);
    assert_line(0, "12:34:56");
    assert_line(1, "Sat 14.02.2026");
    assert_line(2, "0.00C 0.00%");
    unsigned previous_flushes = flush_count;
    next_reading.humidity = 10001;
    assert(display_show_data(display, test_date, next_reading) == DISPLAY_ERROR_ARGUMENT);
    assert(flush_count == previous_flushes);
    puts("climate station tests passed");
    return 0;
}
