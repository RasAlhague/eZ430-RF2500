#include "bsp.h"

#define LOG_NUM_AVGS 2
__no_init volatile int tempOffset @ 0x10F4; // Temperature offset set at production

int get_msp_temperature()
{
  int degC;
  volatile long temp;
  ADC10CTL1 = INCH_10 + ADC10DIV_4;     // Temp Sensor ADC10CLK/5
  ADC10CTL0 = SREF_1 + ADC10SHT_3 + REFON + ADC10ON + ADC10IE + ADC10SR;
  for( degC = 240; degC > 0; degC-- );  // delay to allow reference to settle
  ADC10CTL0 |= ENC + ADC10SC;           // Sampling and conversion start
  __bis_SR_register(CPUOFF + GIE);      // LPM0 with interrupts enabled
  degC = ADC10MEM;
  ADC10CTL0 &= ~ENC;
  // oC = ((A10/1024)*1500mV)-986mV)*1/3.55mV = A10*423/1024 - 278
  // the temperature is transmitted as an integer where 32.1 = 321
  // hence 4230 instead of 423
  temp = degC;
  degC = (((temp - 673) * 4230) / 1024);
  if( tempOffset != 0xFFFF )
  {
    degC += tempOffset; 
  }
  return degC;
}

int get_adc_temperatures(uint8_t input)
{
  int degC;
  volatile long temp;
  ADC10CTL1 = input + ADC10DIV_4;       // select input, ADC10CLK/5
  ADC10CTL0 = SREF_1 + ADC10SHT_3 + REFON + ADC10ON + ADC10IE + ADC10SR;
  for( degC = 240; degC > 0; degC-- );  // delay to allow reference to settle
  ADC10CTL0 |= ENC + ADC10SC;           // Sampling and conversion start
  __bis_SR_register(CPUOFF + GIE);      // LPM0 with interrupts enabled
  degC = ADC10MEM;
  ADC10CTL0 &= ~ENC;
  // oC = ((reading/1024)*1500mV)-500mV)
  temp = degC;
  degC = (((temp*1500)/1024)-500);
  return degC;
}

uint8_t get_battery()
{
  int battery=0;
  ADC10CTL1 = INCH_11;                     // AVcc/2
  ADC10CTL0 = SREF_1 + ADC10SHT_2 + REFON + ADC10ON + ADC10IE + REF2_5V;
  for( battery = 240; battery > 0; battery-- );    // delay to allow reference to settle
  ADC10CTL0 |= ENC + ADC10SC;             // Sampling and conversion start
  __bis_SR_register(CPUOFF + GIE);        // LPM0 with interrupts enabled
  battery = ADC10MEM;
  ADC10CTL0 &= ~ENC;
  ADC10CTL0 &= ~(REFON + ADC10ON);        // turn off A/D to save power
  battery = (battery*25)/512;
  uint8_t batterybyte = battery&0xFF;
  return batterybyte;
}