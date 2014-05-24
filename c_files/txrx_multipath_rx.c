//this code is to be used on the mote connected to the computer. This mote
// 1. receives a number through the serial port indicating the IEEE802.15.4 frequency channel to be used [11..26]
// 2. transmits that same number 10 times over the air, at 0dBm, on channel 26
// 3. listens for incoming packets for 5 sec on the specified channel
// 4. transmits the average RSSI and number of received packets over the serial port

#include "mrfi.h"
#include "radios/family1/mrfi_spi.h"
int32_t cumulative_rssi;
int16_t num_packets_received;
char channel_str[2];
int8_t channel_str_counter;
uint8_t busy;
mrfiPacket_t packet;
void print_probability()
{
  char output[] = {" 000 0000"};
  cumulative_rssi /= num_packets_received;
  if (cumulative_rssi<0) {
    output[0]='-';
    cumulative_rssi=-cumulative_rssi;
  }
  output[1] = '0'+((cumulative_rssi/100)%10);
  output[2] = '0'+((cumulative_rssi/10)%10);
  output[3] = '0'+ (cumulative_rssi%10);
  output[5] = '0'+((num_packets_received/1000)%10);
  output[6] = '0'+((num_packets_received/100)%10);
  output[7] = '0'+((num_packets_received/10)%10);
  output[8] = '0'+ (num_packets_received%10);
  TXString(output, (sizeof output)-1);
}
int main(void)
{
  BSP_Init();
  MRFI_Init();
  P2DIR |= 0x04;
  /* enable serial I/O */
  P3SEL    |= 0x30;
  UCA0CTL1  = UCSSEL_2;
  UCA0BR0   = 0x41;
  UCA0BR1   = 0x3;
  UCA0MCTL  = UCBRS_2;                     
  UCA0CTL1 &= ~UCSWRST;
  IE2      |= UCA0RXIE;
  channel_str_counter=0;
  /* initialize radio */
  mrfiSpiWriteReg(PATABLE,0xFF);// 1dBm Tx power
  mrfiSpiWriteReg(MDMCFG1,0x23);
  mrfiSpiWriteReg(MDMCFG0,0xF8);// 400kHz channel spacing
  mrfiSpiWriteReg(FREQ2,0x5C);
  mrfiSpiWriteReg(FREQ1,0x80);
  mrfiSpiWriteReg(FREQ0,0x00); // 2.405GHz base frequency
  MRFI_WakeUp();
  __bis_SR_register(GIE+LPM3_bits);
}
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
{
  uint8_t channel, i;
  char rx = UCA0RXBUF;
  if (busy==1) {
    return;
  }
  BCSCTL3 |= LFXT1S_2;
  TACCR0=3900; //~3sec
  TACTL=TASSEL_1+MC_1+ID_3+TACLR;
  TACCTL0=CCIE;
  if (rx==';') {
    channel = (uint8_t)((channel_str[0]-'0')*10+(channel_str[1]-'0'));
    if (channel>=11 && channel<=26) {
      busy=1;
      MRFI_RxIdle();
      mrfiSpiWriteReg(CHANNR,0xBC);//channel 26
      for (i=0;i<10;i++) {
        P1OUT |= 0x01;
        P2OUT |= 0x04;
        packet.frame[0]=8+20;
        packet.frame[9]=(uint8_t)channel_str[0];
        packet.frame[10]=(uint8_t)channel_str[1];
        MRFI_Transmit(&packet, MRFI_TX_TYPE_FORCED);
        P2OUT &= ~0x04;
        P1OUT &= ~0x01;
      }
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
      cumulative_rssi=0;
      num_packets_received=0;
      MRFI_RxOn();
    } else {
      TXString("--ERROR--",9);
      TACTL=MC_0;
      TACCTL0=0;
    }
    channel_str_counter=0;
    channel_str[0]=0;
    channel_str[1]=0;
  } else {
    if (channel_str_counter>1) {
       TXString("--ERROR--",9);
       TACTL=MC_0;
       TACCTL0=0;
       channel_str_counter=0;
       channel_str[0]=0;
       channel_str[1]=0;
    } else {
       channel_str[channel_str_counter] = rx;
       channel_str_counter++;
    }
  }
}
void MRFI_RxCompleteISR()
{
  P2OUT |= 0x04;
  P1OUT |= 0x02;
  num_packets_received++;
  MRFI_Receive(&packet);
  cumulative_rssi+=(int8_t) packet.rxMetrics[0];
  P1OUT &= ~0x02;
  P2OUT &= ~0x04;
}
#pragma vector=TIMERA0_VECTOR
__interrupt void Timer_A (void)
{
  TACTL=MC_0;
  TACCTL0=0;
  if (busy==1) {
     P2OUT |= 0x04;
     MRFI_RxIdle();
     print_probability();
     busy=0;
     P2OUT &= ~0x04;
  } else {
     TXString("--ERROR--",9);
     TACTL=MC_0;
     TACCTL0=0;
     channel_str_counter=0;
     channel_str[0]=0;
     channel_str[1]=0;
  }
}