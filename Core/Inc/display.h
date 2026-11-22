#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

#include "stm32f4xx_hal.h"

#ifndef DISPLAY_WIDTH
#define DISPLAY_WIDTH 128U
#endif

#ifndef DISPLAY_HEIGHT
#define DISPLAY_HEIGHT 64U
#endif

typedef enum {
  DISPLAY_OK = 0,
  DISPLAY_ERROR_ARGUMENT,
  DISPLAY_ERROR_NOT_INITIALIZED,
  DISPLAY_ERROR_IO
} display_status_t;

display_status_t display_init(I2C_HandleTypeDef *i2c);
display_status_t display_clear(void);
display_status_t display_draw_text(uint16_t x, uint16_t y, const char *text);
display_status_t display_flush(void);

#endif
