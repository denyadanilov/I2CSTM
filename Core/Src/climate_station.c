#include "climate_station.h"

#include "calendar.h"
#include "climate_station_data.h"
#include "i2c.h"
#include "real_time_clock.h"
#include "temperature_sensor.h"
#include "transmitter.h"

static int climate_handle = 1;

static rtc_handle_t rtc_handle;
static temperature_sensor_handle_t temperature_sensor_handle;

void climate_station_init(climate_station_handle_t* handle) {
  if (*handle == climate_handle) {
    return;
  }

  MX_I2C1_Init();
  transmitter_init();

  if (rtc_init(&rtc_handle, &hi2c1) != RTC_OK) {
    Error_Handler();
  }

  if (temperature_sensor_init(&temperature_sensor_handle, &hi2c1) !=
      TEMPERATURE_SENSOR_OK) {
    Error_Handler();
  }

  *handle = climate_handle;
}

void climate_station_update(void) {
  rtc_date_t date;
  temperature_sensor_data_t reading;

  if (rtc_get_date(rtc_handle, &date) != RTC_OK) {
    Error_Handler();
  }

  if (temperature_sensor_get_data(temperature_sensor_handle, &reading) !=
      TEMPERATURE_SENSOR_OK) {
    Error_Handler();
  }

  if (calendar_weekday(date.year, date.month, date.day) == 0U ||
      date.hours > 23U || date.minutes > 59U || date.seconds > 59U ||
      reading.humidity < 0 || reading.humidity > 10000) {
    Error_Handler();
  }

  climate_station_data_t data = {
      .temperature = (float)reading.temperature / 100.0f,
      .humidity = (float)reading.humidity / 100.0f,
      .date =
          (uint32_t)date.year * 10000U + (uint32_t)date.month * 100U + date.day,
      .time = (uint32_t)date.hours * 10000U + (uint32_t)date.minutes * 100U +
              date.seconds,
  };

  if (transmitter_send(&data, sizeof(data)) != HAL_OK) {
    Error_Handler();
  }
}