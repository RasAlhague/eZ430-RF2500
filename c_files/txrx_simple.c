#include "mrfi.h"
int main(void)
{
  BSP_Init();
  P1REN |= 0x04;
  P1IE |= 0x04;
  MRFI_Init();
  MRFI_WakeUp();
  MRFI_RxOn();
  __bis_SR_register(GIE+LPM4_bits);
}
void MRFI_RxCompleteISR()
{
  P1OUT ^= 0x02;
}
#pragma vector=PORT1_VECTOR
__interrupt void Port_1 (void)
{
  P1IFG &= ~0x04;
  mrfiPacket_t packet;
  packet.frame[0]=8+20;
  MRFI_Transmit(&packet, MRFI_TX_TYPE_FORCED);
  P1OUT ^= 0x01;
}