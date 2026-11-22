#include "climate_station.h"

#include "display.h"
#include "i2c.h"

int climate_station_handle = 1;

void climate_station_init(climate_station_handle_t* handle) {
  MX_I2C1_Init();

  if (display_init(&hi2c1) != DISPLAY_OK ||
      display_draw_text(0, 0, "Hello world") != DISPLAY_OK ||
      display_flush() != DISPLAY_OK) {
    Error_Handler();
  }

  *handle = climate_station_handle;
}