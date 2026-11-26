# I2CSTM climate station

The station reads a DS1307 RTC and BME280 over I2C1, then sends one
`climate_station_data_t` packet per second over SPI1. Failures during sensor
initialization, readings, task creation, or transmission invoke `Error_Handler`.

## Wiring and SPI packet

- I2C1: PB6 SCL, PB7 SDA, with appropriate pull-up resistors.
- SPI1 master: PA5 SCK, PA7 MOSI, PA4 active-low chip select, and common ground.
  The receiver must be an SPI slave and use PA4's low-to-high interval to frame
  each packet. SPI uses mode 0, MSB first, 8-bit words, at 8 MHz with the
  current clock configuration. The receiver must be ready before transmission.
- PA4 starts high, goes low for the entire 16-byte transfer, and returns high
  even if the SPI transfer fails. This is a one-way interface: a successful
  SPI write does not confirm that the receiver received the packet.

The packet is the raw STM32F411 C struct, with this native little-endian layout:

| Offset | Type | Field | Meaning |
| --- | --- | --- | --- |
| 0 | IEEE-754 float32 | temperature | degrees Celsius |
| 4 | IEEE-754 float32 | humidity | percent relative humidity |
| 8 | uint32 | date | decimal `YYYYMMDD`, e.g. 20260203 |
| 12 | uint32 | time | decimal `HHMMSS`, e.g. 40506 for 04:05:06 |

Receiver code must use a compatible layout and byte order. No pressure,
header, checksum, acknowledgment, or retransmission is included.

## BME280 sensor

The default 7-bit I2C address is `0x76`; set
`TEMPERATURE_SENSOR_I2C_ADDRESS` to `0x77` in build flags if the module's SDO
pin selects that address. The sensor API returns temperature in hundredths of
a degree Celsius and humidity in hundredths of percent relative humidity;
the station converts these to floats for transmission. Each read triggers a
fresh forced-mode measurement and blocks for at least 10 ms. Calls must be
serialized with other users of the sensor and I2C bus.

## DS1307 real-time clock

When the oscillator is halted, RTC initialization sets it to the build date
and time compiled into `rtc_build_time.c`. A running clock is left unchanged.
This is the compilation time, not the time of flashing; incremental builds can
reuse an older timestamp.

## Build and host test

CMake requires an ARM `arm-none-eabi` toolchain with the C standard library.
Configure and build using `cmake --preset Debug` and
`cmake --build --preset Debug`.

The packet and station integration test can be run on a little-endian host:

```sh
cc -std=c11 -Itests/stubs -ICore/Inc Core/Src/calendar.c Core/Src/transmitter.c \
  Core/Src/climate_station.c tests/climate_station_test.c -o /tmp/climate-test
/tmp/climate-test
```
