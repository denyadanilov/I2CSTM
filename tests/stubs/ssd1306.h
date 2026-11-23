#ifndef TEST_SSD1306_H
#define TEST_SSD1306_H

#include <stddef.h>
#include <stdint.h>

#define OLED_OK 0
#define OLED_ERR_INVALID_ARG (-1)
#define OLED_ERR_IO (-2)
#define OLED_BUS_I2C 0

typedef struct {
    int bus_type;
    uint16_t width;
    uint16_t height;
    void* user_context;

    union {
        struct {
            uint8_t i2c_address_7bit;
            int32_t (*send_fn)(void*, uint8_t, const uint8_t*, size_t);
        } i2c;
    } transport;
} OLED_Config;

int32_t OLED_Init(const OLED_Config* config);
int32_t OLED_DrawBitmap(const OLED_Config* config, const uint8_t* bitmap, size_t length);

#endif
