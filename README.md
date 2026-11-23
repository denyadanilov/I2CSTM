# I2CSTM

## CMake build

CMake includes the application sources and the STM32-SSD1306 library. By
default it uses the library installed by PlatformIO for the `blackpill_f411ce`
environment. Install that environment's dependencies first, or configure CMake
with `-DSSD1306_DIR=/path/to/STM32-SSD1306`. CMake also requires an ARM
`arm-none-eabi` toolchain with the C standard library.

## BME280 temperature and humidity sensor

The sensor API manages one BME280 on an initialized I2C bus, with the same
handle/status pattern and `Error_Handler` behavior as the RTC. The default
7-bit address is `0x76`; set `TEMPERATURE_SENSOR_I2C_ADDRESS` to `0x77` in
build flags if the module's SDO pin selects that address.

```c
temperature_sensor_handle_t sensor = 0;
temperature_sensor_data_t reading;
if (temperature_sensor_init(&sensor, &hi2c1) != TEMPERATURE_SENSOR_OK ||
    temperature_sensor_get_data(sensor, &reading) != TEMPERATURE_SENSOR_OK) {
  Error_Handler();
}
```

Include `temperature_sensor.h` and `i2c.h` and initialize I2C before calling.
The signed `temperature` value is in hundredths of a degree Celsius and
`humidity` is in hundredths of percent relative humidity (0–10000). Each
read triggers a fresh forced-mode measurement; it blocks for at least 10 ms.
Calls must be serialized with other users of the sensor and I2C bus. This API
is used by the climate station to show temperature and humidity on the third
display line, for example `-0.50C 50.00%`.

The host-side sensor tests can be run with:
```sh
cc -std=c11 -DTEMPERATURE_SENSOR_I2C_ADDRESS=0x76 -Itests/stubs -ICore/Inc \
  Core/Src/temperature_sensor.c tests/temperature_sensor_test.c -o /tmp/bme280-test
/tmp/bme280-test
```

The climate station and display integration tests can be run with:
```sh
cc -std=c11 -Itests/stubs -ICore/Inc Core/Src/calendar.c Core/Src/display.c \
  Core/Src/climate_station.c tests/climate_station_test.c -o /tmp/climate-test
/tmp/climate-test
```

## DS1307 real-time clock

When the oscillator is halted, RTC initialization sets it to the build date
and time compiled into `rtc_build_time.c`. A running clock is left unchanged.
This is the compilation time, not the time of flashing; incremental builds can
reuse an older timestamp.

The clock task reads the RTC and BME280 and updates the display once per second.
RTC, sensor, display, and task creation failures are handled by `Error_Handler`.

## SSD1306 text display

The display component uses I2C address `0x3C` (7-bit) and defaults to a
128x64 display. For a 128x32 display, add `-DDISPLAY_HEIGHT=32` to the
PlatformIO `build_flags`; `DISPLAY_WIDTH` may also be set (up to 128).
Initialize HAL, GPIO and the selected I2C peripheral before calling it.
For I2C1, the configured pins are PB6 (SCL) and PB7 (SDA); the bus requires
appropriate pull-up resistors.

```c
#include "display.h"
#include "i2c.h"
#include "real_time_clock.h"
#include "temperature_sensor.h"

/* After MX_I2C1_Init(): */
display_handle_t display = 0;
rtc_handle_t rtc = 0;
temperature_sensor_handle_t sensor = 0;
rtc_date_t date;
temperature_sensor_data_t reading;
if (display_init(&display, &hi2c1) != DISPLAY_OK) {
  Error_Handler();
}
if (rtc_init(&rtc, &hi2c1) != RTC_OK ||
    temperature_sensor_init(&sensor, &hi2c1) != TEMPERATURE_SENSOR_OK ||
    rtc_get_date(rtc, &date) != RTC_OK ||
    temperature_sensor_get_data(sensor, &reading) != TEMPERATURE_SENSOR_OK ||
    display_show_data(display, date, reading) != DISPLAY_OK) {
  Error_Handler();
}
```

The component manages one display and a framebuffer. `display_clear()` clears
the framebuffer; `display_draw_text(x, y, text)` draws one line of printable
ASCII (space through `~`) at pixel coordinates using an opaque 6x8 cell.
The entire line must fit, including the spacing column, or the call returns
`DISPLAY_ERROR_ARGUMENT` without changing the framebuffer. Newlines and
non-ASCII bytes are not supported. Call `display_flush()` to send buffered
changes to the display. All functions return a `display_status_t`; check for
initialization, argument, and I2C errors. Calls must be serialized by the
application.

`display_show_data(handle, date, reading)` clears the display, shows time and
date on the first two lines and the `temperature_sensor_data_t` reading on the
third line, then flushes the framebuffer. It returns an error for an invalid
date, time, or humidity.
