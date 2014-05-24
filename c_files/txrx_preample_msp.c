#include "mrfi.h"
#include "radios/family1/mrfi_spi.h"
void start_slow_timeout()
{
  TACTL|=TACLR; TACCTL0=CCIE; TACTL=TASSEL_1+MC_1;
}
void stop_slow_timeout()
{
  TACTL=MC_0; TACCTL0=0;
}
void start_fast_timeout()
{
  TBCTL|=TBCLR; TBCCTL0=CCIE; TBCTL=TBSSEL_2+MC_1;
}
void stop_fast_timeout()
{
  TBCTL=MC_0; TBCCTL0=0;
}
void print_counter(int8_t counter)
{
  char output[] = {"   "};
  output[0] = '0'+((counter/10)%10);
  output[1] = '0'+ (counter%10);
  TXString(output, (sizeof output)-1);
}
int main(void)
{
  BSP_Init();
  P1REN |= 0x04;
  P1IE |= 0x04;
  MRFI_Init();
  P3SEL    |= 0x30;     // P3.4,5 = USCI_A0 TXD/RXD
  UCA0CTL1  = UCSSEL_2; // SMCLK
  UCA0BR0   = 0x41;     // 9600 from 8Mhz
  UCA0BR1   = 0x3;
  UCA0MCTL  = UCBRS_2;                     
  UCA0CTL1 &= ~UCSWRST; // Initialize USCI state machine
  IE2      |= UCA0RXIE; // Enable USCI_A0 RX interrupt
  BCSCTL3 |= LFXT1S_2; TACTL=MC_0; TACCTL0=0; TACCR0=1060;  //slow timeout
  TBCTL=MC_0; TBCCTL0=0; TBCCR0=31781;                      //fast timeout
  start_slow_timeout();
  __bis_SR_register(GIE+LPM3_bits);
}
void MRFI_RxCompleteISR()
{
  mrfiPacket_t packet;
  stop_fast_timeout();
  stop_slow_timeout();
  MRFI_Receive(&packet);
  if (packet.frame[9]<4) {
    print_counter(packet.frame[9]);
    start_slow_timeout();
  } else {
    MRFI_WakeUp();
    MRFI_RxOn();
  }
}
#pragma vector=PORT1_VECTOR
__interrupt void interrupt_button (void)
{
  P1IFG &= ~0x04;
  uint8_t counter;
  mrfiPacket_t packet;
  packet.frame[0]=8+20;
  MRFI_WakeUp();
  for (counter=50;counter>=1;counter--) {
     packet.frame[9]=counter;
     MRFI_Transmit(&packet, MRFI_TX_TYPE_FORCED);
  }
}
#pragma vector=TIMERA0_VECTOR
__interrupt void interrupt_slow_timeout (void)
{
  MRFI_WakeUp();
  MRFI_RxOn();
  start_fast_timeout();
  __bic_SR_register_on_exit(SCG1+SCG0);
}
#pragma vector=TIMERB0_VECTOR
__interrupt void interrupt_fast_timeout (void)
{
  stop_fast_timeout();
  MRFI_Sleep();
  __bis_SR_register_on_exit(LPM3_bits);
}