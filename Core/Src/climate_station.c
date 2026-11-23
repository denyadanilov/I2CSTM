#include "climate_station.h"

#include "display.h"
#include "i2c.h"
#include "real_time_clock.h"

static int climate_handle = 1;

static display_handle_t display_handle;
static rtc_handle_t rtc_handle;

void climate_station_init(climate_station_handle_t* handle) {
  if (*handle == climate_handle) {
    return;
  }

  MX_I2C1_Init();

  display_init(&display_handle, &hi2c1);
  rtc_init(&rtc_handle, &hi2c1);

  *handle = climate_handle;
}