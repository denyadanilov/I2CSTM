#ifndef TEST_STM32F4XX_HAL_H
#define TEST_STM32F4XX_HAL_H

#include <stdint.h>

typedef struct {
    int unused;
} I2C_HandleTypeDef;

typedef enum { HAL_OK, HAL_ERROR } HAL_StatusTypeDef;

#define I2C_MEMADD_SIZE_8BIT 1U

HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef* i2c, uint16_t address, uint16_t reg, uint16_t reg_size, uint8_t* data, uint16_t size, uint32_t timeout);
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef* i2c, uint16_t address, uint16_t reg, uint16_t reg_size, uint8_t* data, uint16_t size, uint32_t timeout);
HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef* i2c, uint16_t address, uint8_t* data, uint16_t size, uint32_t timeout);
void HAL_Delay(uint32_t delay);

#endif
