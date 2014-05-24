#include "mrfi.h"
#include "radios/family1/mrfi_spi.h"
void wor_init();
void wor_start();
void wor_stop();
void print_counter(int8_t counter);
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
  wor_init();
  wor_start();
  __bis_SR_register(GIE+LPM4_bits);
}
void MRFI_RxCompleteISR()
{
  mrfiPacket_t packetReceived;
  P1OUT |= 0x02;
  wor_stop();
  MRFI_Receive(&packetReceived);
  if ((MRFI_P_PAYLOAD(&packetReceived))[2]<3) {
    print_counter((MRFI_P_PAYLOAD(&packetReceived))[2]);
    P1OUT &= ~0x02;
    wor_start();
  } else {
    MRFI_WakeUp();
    MRFI_RxOn();
  }
}
#pragma vector=PORT1_VECTOR
__interrupt void Port_1 (void)
{
  P1IFG &= ~0x04;
  uint8_t counter;
  mrfiPacket_t packetToSend;
  MRFI_SET_PAYLOAD_LEN((&packetToSend),20);
  for (counter=50;counter>=1;counter--) {
     P1OUT |= 0x01;
     (MRFI_P_PAYLOAD(&packetToSend))[2] = counter;
     MRFI_Transmit(&packetToSend, MRFI_TX_TYPE_FORCED);
     P1OUT &= ~0x01;
  }
  wor_start();
}
void wor_init()
{
  uint8_t reg;
  /* choose EVENT0 (=(uint16_t)(WOREVT1<<8)+WOREVT10)   == 3600=0x0E10 (103.5ms) */
  mrfiSpiWriteReg(WOREVT1, 0x0E);
  mrfiSpiWriteReg(WOREVT0, 0x10);
  /* choose EVENT1[WORCTRL.6-4]                         == 7=0b111        */
  reg  = mrfiSpiReadReg(WORCTRL);
  reg |=  0x70;
  mrfiSpiWriteReg(WORCTRL, reg);
  /* enable frequence calibration RC_CAL[WORCTRL.3]     == 1              */
  reg  = mrfiSpiReadReg(WORCTRL);
  reg |=  0x80;
  mrfiSpiWriteReg(WORCTRL, reg);
  /* choose WOR_RES[WORCTRL.1-0]                        == 00             */
  reg  = mrfiSpiReadReg(WORCTRL);
  reg &= ~0x03;
  mrfiSpiWriteReg(WORCTRL, reg);
  /* choose when to calibrate FS_AUTOCAL[MCSM0.5-4]     == 01             */
  reg  = mrfiSpiReadReg(MCSM0);
  reg &= ~0x20;
  reg |=  0x10;
  mrfiSpiWriteReg(MCSM0, reg);
  /* initial filter by rssi RX_TIME_RSSI[MCSM2.4]       == 0              */
  reg  = mrfiSpiReadReg(MCSM2);
  reg &= ~0x10;
  mrfiSpiWriteReg(MCSM2, reg);
  /* stay in Rx after preamble RX_TIME_QUAL[MCSM2.3]    == 0              */
  reg  = mrfiSpiReadReg(MCSM2);
  reg &= ~0x08;
  mrfiSpiWriteReg(MCSM2, reg);
  /* enable RC oscilator RC_PD[WORCTRL.7]               == 0              */
  reg  = mrfiSpiReadReg(WORCTRL);
  reg &= ~0x80;
  mrfiSpiWriteReg(WORCTRL, reg);
  /* assert GDO2 at SYNC GDO2_CFG[IOCFG2.5-0]           == 0x06           */
  mrfiSpiWriteReg(IOCFG2, 0x06);          //P14=SYNC
}
void wor_start()
{
  /* choose RX_TIME[MCSM2.2-0]                          == 2=0b010        */
  uint8_t reg  = mrfiSpiReadReg(MCSM2);
  reg &= ~0x05;
  reg |=  0x02;
  //mrfiSpiWriteReg(MCSM2, reg);//test
  mrfiSpiWriteReg(MCSM2, 2);//test
  /* from IDLE state, enter WOR */
  MRFI_RxIdle();
  MRFI_RxWor();
}
void wor_stop()
{
  /* choose RX_TIME[MCSM2.2-0]                          == 7=0b111        */
  uint8_t reg  = mrfiSpiReadReg(MCSM2);
  reg |=  0x07;
  //mrfiSpiWriteReg(MCSM2,reg);
  mrfiSpiWriteReg(MCSM2,7);//test
  /* turn radio to IDLE, quitting WOR*/
  MRFI_RxIdle();
}
void print_counter(int8_t counter)
{
  char output[] = {"   \n"};
  if (((counter/100)%10)>0) {output[0] = '0'+((counter/100)%10);}
  output[1] = '0'+((counter/10)%10);
  output[2] = '0'+ (counter%10);
  TXString(output, (sizeof output)-1);
}