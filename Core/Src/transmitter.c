#include "transmitter.h"

#include <float.h>
#include <stddef.h>

#include "main.h"
#include "spi.h"

#define TRANSMITTER_TIMEOUT_MS 100U

_Static_assert(sizeof(float) == 4 && FLT_RADIX == 2 && FLT_MANT_DIG == 24 &&
                   FLT_MAX_EXP == 128,
               "Transmitter requires IEEE-754 binary32 floats");
_Static_assert(sizeof(climate_station_data_t) == 16 &&
                   offsetof(climate_station_data_t, temperature) == 0 &&
                   offsetof(climate_station_data_t, humidity) == 4 &&
                   offsetof(climate_station_data_t, date) == 8 &&
                   offsetof(climate_station_data_t, time) == 12,
               "Unexpected climate station packet layout");

void transmitter_init(void) { MX_SPI1_Init(); }

HAL_StatusTypeDef transmitter_send(const climate_station_data_t* data,
                                   size_t size) {
  if (data == NULL) {
    return HAL_ERROR;
  }

  HAL_GPIO_WritePin(TRANSMITTER_CS_GPIO_Port, TRANSMITTER_CS_Pin,
                    GPIO_PIN_RESET);
  HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi1, (const uint8_t*)data,
                                              size, TRANSMITTER_TIMEOUT_MS);
  HAL_GPIO_WritePin(TRANSMITTER_CS_GPIO_Port, TRANSMITTER_CS_Pin, GPIO_PIN_SET);
  return status;
}
