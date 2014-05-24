#ifndef SENSING_H
#define SENSING_H

int     get_msp_temperature();
int     get_adc_temperatures(uint8_t input);
uint8_t get_battery();

#endif