#include "mrfi.h"

void print_uint(unsigned int val) {
  char output[] = {"00000 "};
  output[0] = '0'+((val/10000)%10);
  output[1] = '0'+((val/1000)%10);
  output[2] = '0'+((val/100)%10);
  output[3] = '0'+((val/10)%10);
  output[4] = '0'+ (val%10);
  TXString(output, (sizeof output)-1);
}

void ADCinit(void) {
  ADC10CTL1 = INCH_15 + CONSEQ_1 + ADC10SSEL_3;
  ADC10CTL0 = ADC10SHT_3 + MSC + ADC10ON + ADC10IE + REFON + SREF_1;
  ADC10AE0  = 0x3F;    // P2.543210 ADC option select
  ADC10AE1  = 0xF0;    // P4.7654 ADC option select
  ADC10DTC1 = 16;     // One pass through all 16 analog inputs
}

void ADCstart(unsigned int *results) {
  ADC10CTL0 &= ~ENC;
  while (ADC10CTL1 & BUSY);            // Wait if ADC10 core is active
  ADC10SA = (unsigned short)results;   // Data buffer start
  ADC10CTL0 |= ENC + ADC10SC;          // Sampling and conversion ready
}

void UARTinit(void) {
  P3SEL    |= 0x30;     // P3.4,5 = USCI_A0 TXD/RXD
  UCA0CTL1  = UCSSEL_2; // SMCLK
  UCA0BR0   = 0x41;     // 9600 from 8Mhz
  UCA0BR1   = 0x3;
  UCA0MCTL  = UCBRS_2;                     
  UCA0CTL1 &= ~UCSWRST; // Initialize USCI state machine
  IE2      |= UCA0RXIE; // Enable USCI_A0 RX interrupt
}

void timerinit(void) {
  BCSCTL3 &= ~LFXT1S_3;
  BCSCTL3 |= LFXT1S_2;                 // ACLK = VLOCLK (12 kHz)
  BCSCTL1 |= DIVA_3;                   // ACLK /= 8 (1500 Hz)
  //TACCR0 = 1875;                     // 10 seconds
  //TACCR0 = 11250;                    // 1 minute
  TACCR0 = 400;                        // 2 seconds
  TACCTL0 |= CCIE;                     // Compare-mode interrupt
  TACTL = TASSEL_1 + ID_3 + MC_1;      // TACLK = ACLK/8 (187.5 Hz), Up mode
}

void init(void) {
  WDTCTL = WDTPW + WDTHOLD;
  BSP_Init();
  UARTinit();
  ADCinit();
  timerinit();
  P1DIR |=  0x03;
  P1OUT &= ~0x03;
  __bis_SR_register(GIE);
}

void print_temps(unsigned int *temperatures) {
  static int chans[] = {15,14,13,12,11,10,3,2,1,0};
  int i;
  for (i = 0; i < sizeof(chans)/sizeof(int); i++)
    print_uint(temperatures[chans[i]]);
  TXString("\n", 1);
}

#define LOG_NUM_AVGS 2

void avgtemps (unsigned int *temps_avg) {
  unsigned int temps_1[16];
  static unsigned int temps_64[16];
  static int i, j, t;
  for (t = 0; t < 16; t++) {
    temps_64[t] = 0;
    temps_avg[t] = 0;
  }
  for (j = 0; j < (1<<LOG_NUM_AVGS) ; j++) {
    for (i = 0; i < 64; i++) {
      ADCstart(temps_1);
      __bis_SR_register(CPUOFF + GIE);
      for (t = 0; t < 16; t++)
        temps_64[t] += temps_1[t];
    }
    for (t = 0; t < 16; t++) {
      temps_avg[t] += (temps_64[t] >> LOG_NUM_AVGS);
      temps_64[t] = 0;
    }
  }
}

int main( void ) {
  static unsigned int temperatures[16];
  
  init();

  while(1){
    P1OUT |= 0x01;
    avgtemps(temperatures);  
    P1OUT &= ~0x01;
    print_temps(temperatures);
    __bis_SR_register(LPM3_bits + GIE);
  }
}

#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void) {
  __bic_SR_register_on_exit(CPUOFF);        // Clear CPUOFF bit from SR
  P1OUT ^= 0x01;
}

#pragma vector=TIMERA0_VECTOR
__interrupt void TA0_ISR(void) {
  __bic_SR_register_on_exit(LPM3_bits);      // Clear CPUOFF bit from 0(SR)
  P1OUT ^= 0x02;
}

void MRFI_RxCompleteISR()
{
}