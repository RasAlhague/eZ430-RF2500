//******************************************************************************
// THIS PROGRAM IS PROVIDED "AS IS". TI MAKES NO WARRANTIES OR
// REPRESENTATIONS, EITHER EXPRESS, IMPLIED OR STATUTORY,
// INCLUDING ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
// COMPLETENESS OF RESPONSES, RESULTS AND LACK OF NEGLIGENCE.
// TI DISCLAIMS ANY WARRANTY OF TITLE, QUIET ENJOYMENT, QUIET
// POSSESSION, AND NON-INFRINGEMENT OF ANY THIRD PARTY
// INTELLECTUAL PROPERTY RIGHTS WITH REGARD TO THE PROGRAM OR
// YOUR USE OF THE PROGRAM.
//
// IN NO EVENT SHALL TI BE LIABLE FOR ANY SPECIAL, INCIDENTAL,
// CONSEQUENTIAL OR INDIRECT DAMAGES, HOWEVER CAUSED, ON ANY
// THEORY OF LIABILITY AND WHETHER OR NOT TI HAS BEEN ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGES, ARISING IN ANY WAY OUT
// OF THIS AGREEMENT, THE PROGRAM, OR YOUR USE OF THE PROGRAM.
// EXCLUDED DAMAGES INCLUDE, BUT ARE NOT LIMITED TO, COST OF
// REMOVAL OR REINSTALLATION, COMPUTER TIME, LABOR COSTS, LOSS
// OF GOODWILL, LOSS OF PROFITS, LOSS OF SAVINGS, OR LOSS OF
// USE OR INTERRUPTION OF BUSINESS. IN NO EVENT WILL TI'S
// AGGREGATE LIABILITY UNDER THIS AGREEMENT OR ARISING OUT OF
// YOUR USE OF THE PROGRAM EXCEED FIVE HUNDRED DOLLARS
// (U.S.$500).
//
// Unless otherwise stated, the Program written and copyrighted
// by Texas Instruments is distributed as "freeware".  You may,
// only under TI's copyright in the Program, use and modify the
// Program without any charge or restriction.  You may
// distribute to third parties, provided that you transfer a
// copy of this license to the third party and the third party
// agrees to these terms by its first use of the Program. You
// must reproduce the copyright notice and any other legend of
// ownership on each copy or partial copy, of the Program.
//
// You acknowledge and agree that the Program contains
// copyrighted material, trade secrets and other TI proprietary
// information and is protected by copyright laws,
// international copyright treaties, and trade secret laws, as
// well as other intellectual property laws.  To protect TI's
// rights in the Program, you agree not to decompile, reverse
// engineer, disassemble or otherwise translate any object code
// versions of the Program to a human-readable form.  You agree
// that in no event will you alter, remove or destroy any
// copyright notice included in the Program.  TI reserves all
// rights not specifically granted under this license. Except
// as specifically provided herein, nothing in this agreement
// shall be construed as conferring by implication, estoppel,
// or otherwise, upon you, any license or other right under any
// TI patents, copyrights or trade secrets.
//
// You may not use the Program in non-TI devices.
//
//******************************************************************************
//   eZ430-RF2500 Temperature Sensor Access Point
//
//   Description: This is the Access Point software for the eZ430-2500RF
//                Temperature Sensing demo
//
//
//   L. Westlund
//   Version    1.02
//   Texas Instruments, Inc
//   November 2007
//   Built with IAR Embedded Workbench Version: 4.09A
//******************************************************************************
//Change Log:
//******************************************************************************
//Version:  1.02
//Comments: Changed Port toggling to abstract method
//          Removed ToggleLED
//          Fixed comment typos/errors
//          Changed startup string to 1.02
//Version:  1.01
//Comments: Added support for SimpliciTI 1.0.3
//          Changed RSSI read method 
//          Added 3 digit temperature output for 100+F
//          Changed startup string to 1.01
//Version:  1.00
//Comments: Initial Release Version
//******************************************************************************

#include "bsp.h"
#include "mrfi.h"
#include "bsp_leds.h"
#include "bsp_buttons.h"
#include "nwk_types.h"
#include "nwk_api.h"
#include "nwk_frame.h"
#include "nwk.h"

#include "msp430x22x4.h"
#include "vlo_rand.h"

#define MESSAGE_LENGTH 3
void TXString( char* string, int length );
void MCU_Init(void);
void transmitData(int addr, signed char rssi,  char msg[MESSAGE_LENGTH] );
void transmitDataString(char addr[4],char rssi[3], char msg[MESSAGE_LENGTH]);
void createRandomAddress();

//data for terminal output
const char splash[] = {"\r\n--------------------------------------------------\r\n     ****\r\n     ****           eZ430-RF2500\r\n     ******o****    Temperature Sensor Network\r\n********_///_****   Copyright 2007\r\n ******/_//_/*****  Texas Instruments Incorporated\r\n  ** ***(__/*****   All rights reserved.\r\n      *********     Version 1.02\r\n       *****\r\n        ***\r\n--------------------------------------------------\r\n"};

__no_init volatile int tempOffset @ 0x10F4; // Temperature offset set at production
__no_init volatile char Flash_Addr[4] @ 0x10F0; // Flash address set randomly

// reserve space for the maximum possible peer Link IDs
static linkID_t sLID[NUM_CONNECTIONS];
static uint8_t  sNumCurrentPeers;

// callback handler
static uint8_t sCB(linkID_t);

// work loop semaphores
static uint8_t sPeerFrameSem;
static uint8_t sJoinSem;
static uint8_t sSelfMeasureSem;

// mode data verbose = default, deg F = default
char verboseMode = 1;
char degCMode = 0;

void main (void)
{
  addr_t lAddr;
  bspIState_t intState;

  WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT
  {
  // delay loop to ensure proper startup before SimpliciTI increases DCO
  // This is typically tailored to the power supply used, and in this case
  // is overkill for safety due to wide distribution.
    volatile int i;
    for(i = 0; i < 0xFFFF; i++){}
  }
  if( CALBC1_8MHZ == 0xFF )                 // Do not run if cal values are erased
  {
    volatile int i;
    P1DIR |= 0x03;
    BSP_TURN_ON_LED1();
    BSP_TURN_OFF_LED2();
    while(1)
    {
      for(i = 0; i < 0x5FFF; i++){}
      BSP_TOGGLE_LED2();
      BSP_TOGGLE_LED1();
    }
  }
    
  BSP_Init();
  
  if( Flash_Addr[0] == 0xFF && 
      Flash_Addr[1] == 0xFF && 
      Flash_Addr[2] == 0xFF && 
      Flash_Addr[3] == 0xFF )
  {
    createRandomAddress();                  // set Random device address at initial startup
  }
  lAddr.addr[0]=Flash_Addr[0];
  lAddr.addr[1]=Flash_Addr[1];
  lAddr.addr[2]=Flash_Addr[2];
  lAddr.addr[3]=Flash_Addr[3];
  SMPL_Ioctl(IOCTL_OBJ_ADDR, IOCTL_ACT_SET, &lAddr);
  
  MCU_Init();
  //Transmit splash screen and network init notification
  TXString( (char*)splash, sizeof splash);
  TXString( "\r\nInitializing Network....", 26 );

  SMPL_Init(sCB);
  
  // network initialized
  TXString( "Done\r\n", 6);

  // main work loop
  while (1)
  {
    // Wait for the Join semaphore to be set by the receipt of a Join frame from a
    // device that supports and End Device.
    
    if (sJoinSem && (sNumCurrentPeers < NUM_CONNECTIONS))
    {
      // listen for a new connection
      SMPL_LinkListen(&sLID[sNumCurrentPeers]);
      sNumCurrentPeers++;
      BSP_ENTER_CRITICAL_SECTION(intState);
      if (sJoinSem)
      {
        sJoinSem--;
      }
      BSP_EXIT_CRITICAL_SECTION(intState);
    }
    
    // if it is time to measure our own temperature...
    if(sSelfMeasureSem)
    {
      char msg [6];
      char addr[] = {"HUB0"};
      char rssi[] = {"000"};
      int degC, volt;
      volatile long temp;
      int results[2];
      
      ADC10CTL1 = INCH_10 + ADC10DIV_4;     // Temp Sensor ADC10CLK/5
      ADC10CTL0 = SREF_1 + ADC10SHT_3 + REFON + ADC10ON + ADC10IE + ADC10SR;
      for( degC = 240; degC > 0; degC-- );  // delay to allow reference to settle
      ADC10CTL0 |= ENC + ADC10SC;           // Sampling and conversion start
      __bis_SR_register(CPUOFF + GIE);      // LPM0 with interrupts enabled
      results[0] = ADC10MEM;
    
      ADC10CTL0 &= ~ENC;
    
      ADC10CTL1 = INCH_11;                  // AVcc/2
      ADC10CTL0 = SREF_1 + ADC10SHT_2 + REFON + ADC10ON + ADC10IE + REF2_5V;
      for( degC = 240; degC > 0; degC-- );  // delay to allow reference to settle
      ADC10CTL0 |= ENC + ADC10SC;           // Sampling and conversion start
      __bis_SR_register(CPUOFF + GIE);      // LPM0 with interrupts enabled
      results[1] = ADC10MEM;
      ADC10CTL0 &= ~ENC;
      ADC10CTL0 &= ~(REFON + ADC10ON);      // turn off A/D to save power
      
      // oC = ((A10/1024)*1500mV)-986mV)*1/3.55mV = A10*423/1024 - 278
      // the temperature is transmitted as an integer where 32.1 = 321
      // hence 4230 instead of 423
      temp = results[0];
      degC = (((temp - 673) * 4230) / 1024);
      if( tempOffset != 0xFFFF )
      {
        degC += tempOffset; 
      }
      
      temp = results[1];
      volt = (temp*25)/512;
      
      msg[0] = degC&0xFF;
      msg[1] = (degC>>8)&0xFF;
      msg[2] = volt;
      transmitDataString(addr, rssi, msg );
      BSP_TOGGLE_LED1();
      sSelfMeasureSem = 0;
    }
    
    // Have we received a frame on one of the ED connections?
    // No critical section -- it doesn't really matter much if we miss a poll
    if (sPeerFrameSem)
    {
      uint8_t     msg[MAX_APP_PAYLOAD], len, i;

      // process all frames waiting
      for (i=0; i<sNumCurrentPeers; ++i)
      {
        if (SMPL_Receive(sLID[i], msg, &len) == SMPL_SUCCESS)
        {
          ioctlRadioSiginfo_t sigInfo;
          sigInfo.lid = sLID[i];
          SMPL_Ioctl(IOCTL_OBJ_RADIO, IOCTL_ACT_RADIO_SIGINFO, (void *)&sigInfo);
          transmitData( i, (signed char)sigInfo.sigInfo[0], (char*)msg );
          BSP_TOGGLE_LED2();
          BSP_ENTER_CRITICAL_SECTION(intState);
          sPeerFrameSem--;
          BSP_EXIT_CRITICAL_SECTION(intState);
        }
      }
    }
  }
}

/*------------------------------------------------------------------------------
*
------------------------------------------------------------------------------*/
void createRandomAddress()
{
  unsigned int rand, rand2;
  do
  {
    rand = TI_getRandomIntegerFromVLO();    // first byte can not be 0x00 of 0xFF
  }
  while( (rand & 0xFF00)==0xFF00 || (rand & 0xFF00)==0x0000 );
  rand2 = TI_getRandomIntegerFromVLO();
  
  BCSCTL1 = CALBC1_1MHZ;                    // Set DCO to 1MHz
  DCOCTL = CALDCO_1MHZ;
  FCTL2 = FWKEY + FSSEL0 + FN1;             // MCLK/3 for Flash Timing Generator
  FCTL3 = FWKEY + LOCKA;                    // Clear LOCK & LOCKA bits
  FCTL1 = FWKEY + WRT;                      // Set WRT bit for write operation
  
  Flash_Addr[0]=(rand>>8) & 0xFF;
  Flash_Addr[1]=rand & 0xFF;
  Flash_Addr[2]=(rand2>>8) & 0xFF; 
  Flash_Addr[3]=rand2 & 0xFF; 
  
  FCTL1 = FWKEY;                            // Clear WRT bit
  FCTL3 = FWKEY + LOCKA + LOCK;             // Set LOCK & LOCKA bit
}

/*------------------------------------------------------------------------------
*
------------------------------------------------------------------------------*/
void transmitData(int addr, signed char rssi,  char msg[MESSAGE_LENGTH] )
{
  char addrString[4];
  char rssiString[3];
  volatile signed int rssi_int;

  addrString[0] = '0';
  addrString[1] = '0';
  addrString[2] = '0'+(((addr+1)/10)%10);
  addrString[3] = '0'+((addr+1)%10);
  rssi_int = (signed int) rssi;
  rssi_int = rssi_int+128;
  rssi_int = (rssi_int*100)/256;
  rssiString[0] = '0'+(rssi_int%10);
  rssiString[1] = '0'+((rssi_int/10)%10);
  rssiString[2] = '0'+((rssi_int/100)%10);

  transmitDataString( addrString, rssiString, msg );
}

/*------------------------------------------------------------------------------
*
------------------------------------------------------------------------------*/
void transmitDataString(char addr[4],char rssi[3], char msg[MESSAGE_LENGTH] )
{
  char temp_string[] = {" XX.XC"};
  int temp = msg[0] + (msg[1]<<8);

  if( !degCMode )
  {
    temp = (((float)temp)*1.8)+320;
    temp_string[5] = 'F';
  }
  if( temp < 0 )
  {
    temp_string[0] = '-';
    temp = temp * -1;
  }
  else if( ((temp/1000)%10) != 0 )
  {
    temp_string[0] = '0'+((temp/1000)%10);
  }
  temp_string[4] = '0'+(temp%10);
  temp_string[2] = '0'+((temp/10)%10);
  temp_string[1] = '0'+((temp/100)%10);
  
  if( verboseMode )
  {
    char output_verbose[] = {"\r\nNode:XXXX,Temp:-XX.XC,Battery:X.XV,Strength:XXX%,RE:no "};

    output_verbose[46] = rssi[2];
    output_verbose[47] = rssi[1];
    output_verbose[48] = rssi[0];
    
    output_verbose[17] = temp_string[0];
    output_verbose[18] = temp_string[1];
    output_verbose[19] = temp_string[2];
    output_verbose[20] = temp_string[3];
    output_verbose[21] = temp_string[4];
    output_verbose[22] = temp_string[5];
    
    output_verbose[32] = '0'+(msg[2]/10)%10;
    output_verbose[34] = '0'+(msg[2]%10);
    output_verbose[7] = addr[0];
    output_verbose[8] = addr[1];
    output_verbose[9] = addr[2];
    output_verbose[10] = addr[3];
    TXString(output_verbose, sizeof output_verbose );
  }
  else
  {
    char output_short[] = {"\r\n$ADDR,-XX.XC,V.C,RSI,N#"};

    output_short[19] = rssi[2];
    output_short[20] = rssi[1];
    output_short[21] = rssi[0];
    
    
    output_short[8] = temp_string[0];
    output_short[9] = temp_string[1];
    output_short[10] = temp_string[2];
    output_short[11] = temp_string[3];
    output_short[12] = temp_string[4];
    output_short[13] = temp_string[5];
   
    output_short[15] = '0'+(msg[2]/10)%10;
    output_short[17] = '0'+(msg[2]%10);
    output_short[3] = addr[0];
    output_short[4] = addr[1];
    output_short[5] = addr[2];
    output_short[6] = addr[3];
    TXString(output_short, sizeof output_short );
  }
}

/*------------------------------------------------------------------------------
*
------------------------------------------------------------------------------*/
void TXString( char* string, int length )
{
  int pointer;
  for( pointer = 0; pointer < length; pointer++)
  {
    volatile int i;
    UCA0TXBUF = string[pointer];
    while (!(IFG2&UCA0TXIFG));              // USCI_A0 TX buffer ready?
  }
}

/*------------------------------------------------------------------------------
*
------------------------------------------------------------------------------*/
void MCU_Init()
{
  BCSCTL1 = CALBC1_8MHZ;                    // Set DCO
  DCOCTL = CALDCO_8MHZ;
  
  BCSCTL3 |= LFXT1S_2;                      // LFXT1 = VLO
  TACCTL0 = CCIE;                           // TACCR0 interrupt enabled
  TACCR0 = 12000;                           // ~1 second
  TACTL = TASSEL_1 + MC_1;                  // ACLK, upmode
  
  P3SEL |= 0x30;                            // P3.4,5 = USCI_A0 TXD/RXD
  UCA0CTL1 = UCSSEL_2;                      // SMCLK
  UCA0BR0 = 0x41;                           // 9600 from 8Mhz
  UCA0BR1 = 0x3;
  UCA0MCTL = UCBRS_2;                       
  UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
  IE2 |= UCA0RXIE;                          // Enable USCI_A0 RX interrupt
  __enable_interrupt(); 
}
/*------------------------------------------------------------------------------
* Runs in ISR context. Reading the frame should be done in the
* application thread not in the ISR thread.
------------------------------------------------------------------------------*/
static uint8_t sCB(linkID_t lid)
{
  if (lid)
  {
    sPeerFrameSem++;
  }
  else
  {
    sJoinSem++;
  }
  // leave frame to be read by application.
  return 0;
}

/*------------------------------------------------------------------------------
* ADC10 interrupt service routine
------------------------------------------------------------------------------*/
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void)
{
  __bic_SR_register_on_exit(CPUOFF);        // Clear CPUOFF bit from 0(SR)
}

/*------------------------------------------------------------------------------
* Timer A0 interrupt service routine
------------------------------------------------------------------------------*/
#pragma vector=TIMERA0_VECTOR
__interrupt void Timer_A (void)
{
  sSelfMeasureSem = 1;
}

/*------------------------------------------------------------------------------
* USCIA interrupt service routine
------------------------------------------------------------------------------*/
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
{
  char rx = UCA0RXBUF;
  if ( rx == 'V' || rx == 'v' )
  {
    verboseMode = 1;
  }
  else if ( rx == 'M' || rx == 'm' )
  {
    verboseMode = 0;
  }
  else if ( rx == 'F' || rx == 'f' )
  {
    degCMode = 0;
  }
  else if ( rx == 'C' || rx == 'c' )
  {
    degCMode = 1;
  }
}
