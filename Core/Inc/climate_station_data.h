#ifndef CLIMATE_STATION_DATA_H
#define CLIMATE_STATION_DATA_H

#include <stdint.h>

typedef struct {
  float temperature;
  float humidity;
  uint32_t date;
  uint32_t time;
} climate_station_data_t;

#endif