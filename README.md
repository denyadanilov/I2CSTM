# I2CSTM

## DS1307 real-time clock

When the oscillator is halted, RTC initialization sets it to the build date
and time compiled into `rtc_build_time.c`. A running clock is left unchanged.
This is the compilation time, not the time of flashing; incremental builds can
reuse an older timestamp.

Run the RTC host test with:

```sh
cc -std=c11 -Wall -Wextra -Werror -Itest/rtc -ICore/Inc \
  test/rtc/test_real_time_clock.c Core/Src/real_time_clock.c \
  Core/Src/rtc_build_time.c Core/Src/calendar.c \
  -o /tmp/i2cstm-rtc-test && /tmp/i2cstm-rtc-test
```

## SSD1306 text display

The display component uses I2C address `0x3C` (7-bit) and defaults to a
128x64 display. For a 128x32 display, add `-DDISPLAY_HEIGHT=32` to the
PlatformIO `build_flags`; `DISPLAY_WIDTH` may also be set (up to 128).
Initialize HAL, GPIO and the selected I2C peripheral before calling it.
For I2C1, the configured pins are PB6 (SCL) and PB7 (SDA); the bus requires
appropriate pull-up resistors.

The climate station reads an SHT30 on I2C1 and displays temperature in Celsius
and relative humidity, refreshed every two seconds. If a measurement or its CRC
fails, it displays `Sensor error` and retries at the next refresh. Display
errors are handled by `Error_Handler`.

For I2C failures, the display also shows whether the command write or data
read failed, the 7-bit address, the HAL status and the HAL I2C error bitmask.
`HAL 1 E 00000004` means `HAL_ERROR` with acknowledge failure (NACK): check
the address, power and wiring. `CRC mismatch` indicates that data was
received but failed its checksum. The last I2C failure details are also
available in `sensor_t` and reset on the next read.

The sensor defaults to the 7-bit I2C address `0x44` (ADDR low). If ADDR is
high, set PlatformIO `build_flags = -DCLIMATE_SENSOR_ADDRESS=0x45`. Connect
the sensor and display to the same SDA/SCL bus and use suitable pull-ups and
logic voltage for the STM32. The sensor API accepts either address and returns
temperature in hundredths of a degree Celsius and humidity in hundredths of a
percent. Readings are updated only after both CRC checks succeed.

```c
#include "display.h"
#include "i2c.h"

/* After MX_I2C1_Init(): */
if (display_init(&hi2c1) != DISPLAY_OK ||
    display_draw_text(0, 0, "Hello, world!") != DISPLAY_OK ||
    display_flush() != DISPLAY_OK) {
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

`display_show_date(date)` clears the display and shows a supplied `rtc_date_t`
as `HH:MM:SS` and `Sat 14.02.2026` on two lines, then flushes the framebuffer.
It returns an error for an invalid date or time.

Run the date display test with:

```sh
cc -std=c11 -Wall -Wextra -Werror -Itest/display -ICore/Inc \
  -I.pio/libdeps/blackpill_f411ce/STM32-SSD1306/include \
  test/display/test_display_date.c Core/Src/display.c Core/Src/calendar.c \
  -o /tmp/i2cstm-display-date-test && /tmp/i2cstm-display-date-test
```

Host-side display tests can be run after PlatformIO has installed the library:

```sh
cc -std=c11 -Wall -Wextra -Werror \
  -Itest/display -ICore/Inc \
  -I.pio/libdeps/blackpill_f411ce/STM32-SSD1306/include \
  test/display/test_display.c Core/Src/display.c Core/Src/calendar.c \
  .pio/libdeps/blackpill_f411ce/STM32-SSD1306/src/ssd1306.c \
  -o /tmp/i2cstm-display-test && /tmp/i2cstm-display-test
```

Run the sensor and station tests with:

```sh
cc -std=c11 -Wall -Wextra -Werror -Itest/display -ICore/Inc \
  test/sensor/test_sensor.c Core/Src/sensor.c -o /tmp/i2cstm-sensor-test \
  && /tmp/i2cstm-sensor-test
cc -std=c11 -Wall -Wextra -Werror -Itest/climate_station -Itest/display -ICore/Inc \
  test/climate_station/test_climate_station.c Core/Src/climate_station.c \
  -o /tmp/i2cstm-station-test && /tmp/i2cstm-station-test
```
