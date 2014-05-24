#include "radios/family1/mrfi_spi.h"
#include "mrfi.h"
mrfiPacket_t packetToSend;
int main(void)
{
  BSP_Init();
  P1REN |= 0x04;
  P1IE |= 0x04;
  MRFI_Init();
  mrfiSpiWriteReg(PATABLE,0xBB);
  mrfiSpiWriteReg(PKTCTRL0,0x41);
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
  uint8_t i;
  P1OUT ^= 0x02;
  mrfiPacket_t packet;
  MRFI_Receive(&packet);
  char output[] = {"                         \r\n"};
  for (i=9;i<packet.frame[0];i++) {
    output[i-9]=packet.frame[i];
  }
  TXString(output, (sizeof output));
}
#pragma vector=PORT1_VECTOR
__interrupt void Port_1 (void)
{
  P1IFG &= ~0x04;
  char character = 'a';
  uint8_t index;
  mrfiPacket_t packet;
  for (index=9;index<30;index++) {
    packet.frame[index]=character++;
  }
  packet.frame[0]=++index;
  MRFI_Transmit(&packet, MRFI_TX_TYPE_FORCED);
  P1OUT ^= 0x01;
}