#include "mrfi.h"
#include "radios/family1/mrfi_spi.h"

void print_rssi(int8_t rssi)
{
  char output[] = {" 000 "};
  if (rssi<0) {output[0]='-';rssi=-rssi;}
  output[1] = '0'+((rssi/100)%10);
  output[2] = '0'+((rssi/10)%10);
  output[3] = '0'+ (rssi%10);
  TXString(output, (sizeof output)-1);
}

void SwitchGreenLED()
{
  P1OUT ^= 0x02;
}

void SwitcRedLED()
{
  P1OUT ^= 0x01;
}
/**
channelNumber must be <=255
*/
void ReadChannelsAndSendRSSI(int channelNumber)
{
  uint8_t channel;
  int8_t rssi;
  
  for (channel=0;channel<channelNumber;channel++)
  {
    MRFI_RxIdle();
    mrfiSpiWriteReg(CHANNR,channel);
    MRFI_RxOn();
    rssi=MRFI_Rssi();
    print_rssi(rssi);
  }
}

void SetBaseFrequencyRegisters(uint8_t range)
{
    MRFI_RxIdle();
    
    //if Channel spacing = 25.390625 (min)
    switch(range)
    {
        // 2399.999634
      case 1:
        mrfiSpiWriteReg(FREQ0,0xC4);
        mrfiSpiWriteReg(FREQ1,0x4E);
        mrfiSpiWriteReg(FREQ2,0x5C);
        break;
        
        //needed 2406.474243
        //seted 2406.473846
      case 2:
        mrfiSpiWriteReg(FREQ0,0x83);
        mrfiSpiWriteReg(FREQ1,0x8E);
        mrfiSpiWriteReg(FREQ2,0x5C);
        break;
        
        //needed 2412.948456
        //seted 2412.948456
      case 3:
        mrfiSpiWriteReg(FREQ0,0x43);
        mrfiSpiWriteReg(FREQ1,0xCE);
        mrfiSpiWriteReg(FREQ2,0x5C);
        break;
        
        //needed 2419.423065
        //seted 2419.423065
      case 4:
        mrfiSpiWriteReg(FREQ0,0x03);
        mrfiSpiWriteReg(FREQ1,0x0E);
        mrfiSpiWriteReg(FREQ2,0x5D);
        break;
        
        //needed 2425.897675
        //seted 2425.897675
      case 5:
        mrfiSpiWriteReg(FREQ0,0xC3);
        mrfiSpiWriteReg(FREQ1,0x4D);
        mrfiSpiWriteReg(FREQ2,0x5D);
        break;

        //needed 2432.372284
        //seted 2432.372284
      case 6:
        mrfiSpiWriteReg(FREQ0,0x83);
        mrfiSpiWriteReg(FREQ1,0x8D);
        mrfiSpiWriteReg(FREQ2,0x5D);
        break;
        
        //needed 2438.846893
        //seted 2438.846893
      case 7:
        mrfiSpiWriteReg(FREQ0,0x43);
        mrfiSpiWriteReg(FREQ1,0xCD);
        mrfiSpiWriteReg(FREQ2,0x5D);
        break;
        
        //needed 2445.321503
        //seted 2445.321503
      case 8:
        mrfiSpiWriteReg(FREQ0,0x03);
        mrfiSpiWriteReg(FREQ1,0x0D);
        mrfiSpiWriteReg(FREQ2,0x5E);
        break;
        
        //needed 2451.796112
        //seted 2451.796112
      case 9:
        mrfiSpiWriteReg(FREQ0,0xC3);
        mrfiSpiWriteReg(FREQ1,0x4C);
        mrfiSpiWriteReg(FREQ2,0x5E);
        break;
        
        //needed 2458.270721
        //seted 2458.270721
      case 10:
        mrfiSpiWriteReg(FREQ0,0x83);
        mrfiSpiWriteReg(FREQ1,0x8C);
        mrfiSpiWriteReg(FREQ2,0x5E);
        break;
        
        //needed 2464.745331
        //seted 2464.745331
      case 11:
        mrfiSpiWriteReg(FREQ0,0x43);
        mrfiSpiWriteReg(FREQ1,0xCC);
        mrfiSpiWriteReg(FREQ2,0x5E);
        break;
        
        //needed 2471.219940
        //seted 2471.219940
      case 12:
        mrfiSpiWriteReg(FREQ0,0x03);
        mrfiSpiWriteReg(FREQ1,0x0C);
        mrfiSpiWriteReg(FREQ2,0x5F);
        break;
        
        //needed 2477.694550
        //seted 2477.694550
        //with chanell 228 carrier freq = 2483.483612 (max)
      case 13:
        mrfiSpiWriteReg(FREQ0,0xC3);
        mrfiSpiWriteReg(FREQ1,0x4B);
        mrfiSpiWriteReg(FREQ2,0x5F);
        break;
        
        
        // 2399.999634
      default:
        mrfiSpiWriteReg(FREQ0,0xC4);
        mrfiSpiWriteReg(FREQ1,0x4E);
        mrfiSpiWriteReg(FREQ2,0x5C);
        break;
    }
}


void letsRock()
{
  BSP_Init();
  MRFI_Init();
  P3SEL    |= 0x30;
  UCA0CTL1  = UCSSEL_2;
  UCA0BR0   = 0x41;
  UCA0BR1   = 0x3;
  UCA0MCTL  = UCBRS_2;
  UCA0CTL1 &= ~UCSWRST;
  //IE2      |= UCA0RXIE; // Enable USCI_A0 RX interrupt
  MRFI_WakeUp();
  __bis_SR_register(GIE);
  
  //Need to check this field if smartrf_CC2500.h is changed 
  int channellToScanQuantity = 255;
  
  while(1)
    {
      SwitchGreenLED();
      ReadChannelsAndSendRSSI(channellToScanQuantity);
      TXString("\n",1);
    }
}

/*

Register VALUES stores in smartrf_CC2500.h
Register ADDRESS stores in mrfi_spi.h

*/

int loop = 1;
int main(void)
{
  //wait ~ 10sec 
  WDTCTL = WDTPW + WDTHOLD;
  int i;
  int y;
    for (i=0;i<100;i++) {
      for (y=0;y<10000;y++) {
        __no_operation();
      }
      for (y=0;y<10000;y++) {
        __no_operation();
      }
    }
  
  //and 
  letsRock();
  
  /*
  SubChannelsAmount depended channel spacing (MDMCFG0 and MDMCFG1)
  for SubChannelsAmount = 13 -- Channel spacing = 25.390625 (min) 
  for SubChannelsAmount = 1 -- Channel spacing = 326.904297 (min) (9C 23)
  */
  /*
  uint8_t SubChannelsAmount = 1;
  
  if(SubChannelsAmount == 1)
  {
    while(1)
    {
      SwitchGreenLED();
      ReadChannelsAndSendRSSI();
      TXString("\n",1);
    }
  }
  else
  {
    while(1)
    {
      for(uint8_t SubChannelsCounter = 1; SubChannelsCounter <= SubChannelsAmount; SubChannelsCounter++)
      {
          SetBaseFrequencyRegisters(SubChannelsCounter);
          ReadChannelsAndSendRSSI();
          SwitchGreenLED();
          TXString("\n",1);
      }
    }
  }*/
}

void MRFI_RxCompleteISR()
{
}

#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
{
  /*
    char rx = UCA0RXBUF;

    if (rx=='\r')
    {
      letsRock();
    }
  */
}