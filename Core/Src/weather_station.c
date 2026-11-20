#include "weather_station.h"

#include "i2c.h"

int weather_station_handle = 1;

void weather_station_init(weather_station_handle_t* handle) {
  MX_I2C1_Init();
  *handle = weather_station_handle;
}