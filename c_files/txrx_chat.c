#include "radios/family1/mrfi_spi.h"
#include "mrfi.h"
uint8_t index_output = 9;
mrfiPacket_t packetToSend;
int main(void)
{
  BSP_Init();
  MRFI_Init();
  P3SEL    |= 0x30;     // P3.4,5 = USCI_A0 TXD/RXD
  UCA0CTL1  = UCSSEL_2; // SMCLK
  UCA0BR0   = 0x41;     // 9600 from 8Mhz
  UCA0BR1   = 0x3;
  UCA0MCTL  = UCBRS_2;                     
  UCA0CTL1 &= ~UCSWRST; // Initialize USCI state machine
  IE2      |= UCA0RXIE; // Enable USCI_A0 RX interrupt
  MRFI_WakeUp();
  MRFI_RxOn();
  index_output=0;
  __bis_SR_register(GIE+LPM4_bits);
}
void MRFI_RxCompleteISR()
{
  uint8_t i;
  P1OUT ^= 0x02;
  mrfiPacket_t packet;
  MRFI_Receive(&packet);
  char output[] = {"                   "};
  for (i=9;i<29;i++) {
    output[i-9]=packet.frame[i];
    if (packet.frame[i]=='\r') {
      output[i-9]='\n';
      output[i-8]='\r';
    }
  }
  TXString(output, (sizeof output));
}
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
{
  char rx = UCA0RXBUF;
  uint8_t i;
  packetToSend.frame[index_output]=rx;
  index_output++;
  if (rx=='\r' || index_output==29) {
     packetToSend.frame[0]=28;
     MRFI_Transmit(&packetToSend, MRFI_TX_TYPE_FORCED);
     index_output = 9;
     for(i=9;i<29;i++) {
       packetToSend.frame[i]=' ';
     }
     P1OUT ^= 0x01;
  }
  P1OUT ^= 0x02;
  TXString(&rx, 1);
}