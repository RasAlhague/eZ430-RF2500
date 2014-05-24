//this code is to be used on the mote mounted on the x-y stage. This mote
// 0. listens on channel 26
// 1. receives a number over the air indicating the IEEE802.15.4 frequency channel to be used [11..26]
// 2. wait for 1 sec after receiving the last number while switching to that channel
// 3. transmits 1000 packets of the air containing 1 byte of bogus data at -30dBm, on the specified channel
// 4. resumes listening on channel 26

#include "mrfi.h"
#include "radios/family1/mrfi_spi.h"
mrfiPacket_t packet;
int main(void)
{
  BSP_Init();
  P2DIR |= 0x04;
  MRFI_Init();
  //mrfiSpiWriteReg(PATABLE,0x50);// -30dBm Tx power
  //mrfiSpiWriteReg(PATABLE,0x84);// -24dBm Tx power
  //mrfiSpiWriteReg(PATABLE,0x46);// -20dBm Tx power
  //mrfiSpiWriteReg(PATABLE,0x55);// -16dBm Tx power
  //mrfiSpiWriteReg(PATABLE,0x8D);// -14dBm Tx power
  //mrfiSpiWriteReg(PATABLE,0xC6);// -12dBm Tx power
  //mrfiSpiWriteReg(PATABLE,0x97);// -10dBm Tx power
  //mrfiSpiWriteReg(PATABLE,0x6E);// -8 dBm Tx power
  //mrfiSpiWriteReg(PATABLE,0x7F);// -6 dBm Tx power
  //mrfiSpiWriteReg(PATABLE,0xA9);// -4 dBm Tx power
  //mrfiSpiWriteReg(PATABLE,0xBB);// -2 dBm Tx power
  mrfiSpiWriteReg(PATABLE,0xFE);// 0dBm Tx power
  mrfiSpiWriteReg(MDMCFG1,0x23);
  mrfiSpiWriteReg(MDMCFG0,0xF8);// 400kHz channel spacing
  mrfiSpiWriteReg(FREQ2,0x5C);
  mrfiSpiWriteReg(FREQ1,0x80);
  mrfiSpiWriteReg(FREQ0,0x00);  // 2.405GHz base frequency
  mrfiSpiWriteReg(CHANNR,0xBC); // channel 26
  MRFI_WakeUp();
  MRFI_RxOn();
  __bis_SR_register(GIE+LPM3_bits);
}
void MRFI_RxCompleteISR()
{
  P1OUT |= 0x02;
  P2OUT |= 0x04;
  uint8_t erase_me=0;
  uint8_t channel;
  MRFI_Receive(&packet);
  channel = (uint8_t)((packet.frame[9]-'0')*10+(packet.frame[10]-'0'));
  erase_me++;
  if (channel>=11 && channel<=26) {
     /* set timer */
     BCSCTL3 |= LFXT1S_2;
     TACCR0=1000; //~.1sec
     TACTL=TASSEL_1+MC_1;
     TACCTL0=CCIE;
     /* set frequency */
     MRFI_RxIdle();
     switch(channel) {
        case 11: mrfiSpiWriteReg(CHANNR,0x00); break;
        case 12: mrfiSpiWriteReg(CHANNR,0x0D); break;
        case 13: mrfiSpiWriteReg(CHANNR,0x19); break;
        case 14: mrfiSpiWriteReg(CHANNR,0x26); break;
        case 15: mrfiSpiWriteReg(CHANNR,0x32); break;
        case 16: mrfiSpiWriteReg(CHANNR,0x3F); break;
        case 17: mrfiSpiWriteReg(CHANNR,0x4B); break;
        case 18: mrfiSpiWriteReg(CHANNR,0x58); break;
        case 19: mrfiSpiWriteReg(CHANNR,0x64); break;
        case 20: mrfiSpiWriteReg(CHANNR,0x71); break;
        case 21: mrfiSpiWriteReg(CHANNR,0x7D); break;
        case 22: mrfiSpiWriteReg(CHANNR,0x8A); break;
        case 23: mrfiSpiWriteReg(CHANNR,0x96); break;
        case 24: mrfiSpiWriteReg(CHANNR,0xA3); break;
        case 25: mrfiSpiWriteReg(CHANNR,0xAF); break;
        case 26: mrfiSpiWriteReg(CHANNR,0xBC); break;
     }
  }
  P2OUT &= ~0x04;
  P1OUT &= ~0x02;
}
#pragma vector=TIMERA0_VECTOR
__interrupt void Timer_A (void)
{
  P2OUT |= 0x04;
  uint16_t i;
  /* stop timer */
  TACTL=MC_0;
  TACCTL0=0;
  /* send probe packet */
  for (i=0;i<1000;i++) {
     P1OUT |=  0x01;
     packet.frame[0]=8+20;
     MRFI_Transmit(&packet, MRFI_TX_TYPE_FORCED);
     P1OUT &= ~0x01;
  }
  /* return to Rx mode on channel 26 */
  MRFI_RxIdle();
  mrfiSpiWriteReg(CHANNR,0xBC); // channel 26
  MRFI_RxOn();
  P2OUT &= ~0x04;
}