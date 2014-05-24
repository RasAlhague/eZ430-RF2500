#include "mrfi.h"

uint8_t get_adc(int input)
{
  uint8_t value;
  volatile long temp;
  ADC10CTL1 = input + ADC10DIV_4;       // select input, ADC10CLK/5
  ADC10CTL0 = SREF_1 + ADC10SHT_3 + REFON + ADC10ON + ADC10IE + ADC10SR;
  for( value = 240; value > 0; value-- );  // delay to allow reference to settle
  ADC10CTL0 |= ENC + ADC10SC;           // Sampling and conversion start
  __bis_SR_register(CPUOFF + GIE);      // LPM0 with interrupts enabled
  value = ADC10MEM;
  ADC10CTL0 &= ~ENC;
  return value;
}

int main( void )
{
  int value0,value1,value2,pointer;
  BSP_Init();
  P2DIR |= 0x02;
  P3SEL    |= 0x30;     // P3.4,5 = USCI_A0 TXD/RXD
  UCA0CTL1  = UCSSEL_2; // SMCLK
  UCA0BR0   = 0x41;     // 9600 from 8Mhz
  UCA0BR1   = 0x3;
  UCA0MCTL  = UCBRS_2;
  //UCA0BR0   = 0xA0;     // 19200 from 8Mhz
  //UCA0BR1   = 0x1;
  //UCA0MCTL  = UCBRS_6;
  //UCA0BR0   = 0x45;     // 115200 from 8Mhz
  //UCA0BR1   = 0x0;
  //UCA0MCTL  = UCBRS_4;
  //UCA0BR0   = 0x1F;     // 256000 from 8Mhz
  //UCA0BR1   = 0x0;
  //UCA0MCTL  = UCBRS_2;
  UCA0CTL1 &= ~UCSWRST; // Initialize USCI state machine
  IE2      |= UCA0RXIE; // Enable USCI_A0 RX interrupt
  __bis_SR_register(GIE);
  char output[] = {"              \r\n"};
  while(1) {
    P2OUT ^= 0x02;
    value0 = get_adc(0);
    output[0] = '0'+((value0/1000)%10);
    output[1] = '0'+((value0/100)%10);
    output[2] = '0'+((value0/10)%10);
    output[3] = '0'+(value0%10);
    value1 = get_adc(1);
    output[5] = '0'+((value1/1000)%10);
    output[6] = '0'+((value1/100)%10);
    output[7] = '0'+((value1/10)%10);
    output[8] = '0'+(value1%10);
    value2 = get_adc(2);
    output[10] = '0'+((value2/1000)%10);
    output[11] = '0'+((value2/100)%10);
    output[12] = '0'+((value2/10)%10);
    output[13] = '0'+(value2%10);
    for( pointer = 0; pointer < 16; pointer++)
    {
      volatile int i;
      UCA0TXBUF = output[pointer];
      while (!(IFG2&UCA0TXIFG));              // USCI_A0 TX buffer ready?
    }
  }
}
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void) {
  __bic_SR_register_on_exit(CPUOFF);        // Clear CPUOFF bit from SR
  P1OUT ^= 0x02;
}
void MRFI_RxCompleteISR()
{
}