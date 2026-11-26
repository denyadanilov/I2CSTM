#ifndef TRANSMITTER_H
#define TRANSMITTER_H

#include "climate_station_data.h"
#include "stm32f4xx_hal.h"

void transmitter_init(void);
HAL_StatusTypeDef transmitter_send(const climate_station_data_t* data,
                                   size_t size);

#endif
