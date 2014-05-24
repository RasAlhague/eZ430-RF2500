#include "mrfi.h"
uint8_t counter, num_received, bool_counting;
int16_t cumulative_rssi;
mrfiPacket_t packet;
void print_probability(int16_t cumulative_rssi, uint8_t number)
{
  char output[] = {" 000 0.00\n"};
  if (cumulative_rssi<0) {
    output[0]='-';
    cumulative_rssi=-cumulative_rssi;
  }
  output[1] = '0'+((cumulative_rssi/100)%10);
  output[2] = '0'+((cumulative_rssi/10)%10);
  output[3] = '0'+ (cumulative_rssi%10);
  output[5] = '0'+((number/100)%10);
  output[7] = '0'+((number/10)%10);
  output[8] = '0'+ (number%10);
  TXString(output, (sizeof output)-1);
}
int main(void)
{
  BSP_Init();
  P1REN |= 0x04;
  P1IE  |= 0x04;
  MRFI_Init();
  mrfiSpiWriteReg(PATABLE,0x50);
  P3SEL    |= 0x30;
  UCA0CTL1  = UCSSEL_2;
  UCA0BR0   = 0x41;
  UCA0BR1   = 0x3;
  UCA0MCTL  = UCBRS_2;                     
  UCA0CTL1 &= ~UCSWRST;
  MRFI_WakeUp();
  MRFI_RxOn();
  __bis_SR_register(GIE+LPM4_bits);
}
void MRFI_RxCompleteISR()
{
  P1OUT ^= 0x02;
  MRFI_Receive(&packet);
  counter = packet.frame[9];
  if (counter==101) {
    if (bool_counting == 1) {
      print_probability(cumulative_rssi/num_received,num_received);
    }
    bool_counting=0;
    num_received=0;
    cumulative_rssi=0;
  } else {
    bool_counting=1;
    num_received++;
    cumulative_rssi+=(int8_t) packet.rxMetrics[0];
  }
}
#pragma vector=PORT1_VECTOR
__interrupt void interrupt_button (void)
{
  P1IFG &= ~0x04;
  P1OUT ^= 0x01;
  mrfiPacket_t packet;
  packet.frame[0]=8+3;
  for (counter=1;counter<101;counter++){
     packet.frame[9]=counter;
     MRFI_Transmit(&packet, MRFI_TX_TYPE_FORCED);
  }
  for (counter=0;counter<20;counter++){
     packet.frame[9]=101;
     MRFI_Transmit(&packet, MRFI_TX_TYPE_FORCED);
  }
}