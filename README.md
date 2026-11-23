# I2CSTM

## DS1307 real-time clock

When the oscillator is halted, RTC initialization sets it to the build date
and time compiled into `rtc_build_time.c`. A running clock is left unchanged.
This is the compilation time, not the time of flashing; incremental builds can
reuse an older timestamp.

The clock task reads the RTC and updates the display once per second. RTC,
display, and task creation failures are handled by `Error_Handler`.

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

/* After MX_I2C1_Init(): */
display_handle_t display = 0;
rtc_handle_t rtc = 0;
if (display_init(&display, &hi2c1) != DISPLAY_OK) {
  Error_Handler();
}
rtc_init(&rtc, &hi2c1);
if (display_show_date(display, rtc_get_date(rtc)) != DISPLAY_OK) {
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

`display_show_date(handle, date)` clears the display and shows a supplied `rtc_date_t`
as `HH:MM:SS` and `Sat 14.02.2026` on two lines, then flushes the framebuffer.
It returns an error for an invalid date or time.
