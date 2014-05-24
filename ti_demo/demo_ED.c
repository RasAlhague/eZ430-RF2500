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
//   eZ430-RF2500 Temperature Sensor End Device
//
//   Description: This is the End Device software for the eZ430-2500RF
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
//          Fixed comment typos
//Version:  1.01
//Comments: Added support for SimpliciTI 1.0.3
//          Added Flash storage/check of Random address
//          Moved LED toggle to HAL
//Version:  1.00
//Comments: Initial Release Version
//******************************************************************************

#include "bsp.h"
#include "mrfi.h"
#include "nwk_types.h"
#include "nwk_api.h"
#include "bsp_leds.h"
#include "bsp_buttons.h"
#include "vlo_rand.h"

void linkTo(void);
void MCU_Init(void);

__no_init volatile int tempOffset @ 0x10F4; // Temperature offset set at production
__no_init volatile char Flash_Addr[4] @ 0x10F0; // Flash address set randomly

void createRandomAddress(); 

void main (void)
{
  addr_t lAddr;
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
  
  // SimpliciTI will change port pin settings as well
  P1DIR = 0xFF;
  P1OUT = 0x00;
  P2DIR = 0x27;
  P2OUT = 0x00;
  P3DIR = 0xC0;
  P3OUT = 0x00;
  P4DIR = 0xFF;
  P4OUT = 0x00;
  
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
  BCSCTL1 = CALBC1_8MHZ;                    // Set DCO after random function
  DCOCTL = CALDCO_8MHZ;
  
  BCSCTL3 |= LFXT1S_2;                      // LFXT1 = VLO
  TACCTL0 = CCIE;                           // TACCR0 interrupt enabled
  TACCR0 = 12000;                           // ~ 1 sec
  TACTL = TASSEL_1 + MC_1;                  // ACLK, upmode
  
  // keep trying to join until successful. toggle LEDS to indicate that
  // joining has not occurred. LED3 is red but labeled LED 4 on the EXP
  // board silkscreen. LED1 is green.
  while (SMPL_NO_JOIN == SMPL_Init((uint8_t (*)(linkID_t))0))
  {
    BSP_TOGGLE_LED1();
    BSP_TOGGLE_LED2();;
    __bis_SR_register(LPM3_bits + GIE);     // LPM3 with interrupts enabled
  }
  // unconditional link to AP which is listening due to successful join.
  linkTo();
}

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

void linkTo()
{
  linkID_t linkID1;
  uint8_t  msg[3];

  // keep trying to link...
  while (SMPL_SUCCESS != SMPL_Link(&linkID1))
  {
    __bis_SR_register(LPM3_bits + GIE);     // LPM3 with interrupts enabled
    BSP_TOGGLE_LED1();
    BSP_TOGGLE_LED2();
  }
   
  // Turn off all LEDs
  if (BSP_LED1_IS_ON())
  {
    BSP_TOGGLE_LED1();
  }
  if (BSP_LED2_IS_ON())
  {
    BSP_TOGGLE_LED2();
  }
  while (1)
  {
    volatile long temp;
    int degC, volt;
    int results[2];
    SMPL_Ioctl( IOCTL_OBJ_RADIO, IOCTL_ACT_RADIO_SLEEP, "" );
    __bis_SR_register(LPM3_bits+GIE);       // LPM3 with interrupts enabled
    SMPL_Ioctl( IOCTL_OBJ_RADIO, IOCTL_ACT_RADIO_AWAKE, "" );
    
    BSP_TOGGLE_LED2();
    ADC10CTL1 = INCH_10 + ADC10DIV_4;       // Temp Sensor ADC10CLK/5
    ADC10CTL0 = SREF_1 + ADC10SHT_3 + REFON + ADC10ON + ADC10IE + ADC10SR;
    for( degC = 240; degC > 0; degC-- );    // delay to allow reference to settle
    ADC10CTL0 |= ENC + ADC10SC;             // Sampling and conversion start
    __bis_SR_register(CPUOFF + GIE);        // LPM0 with interrupts enabled
    results[0] = ADC10MEM;
    
    ADC10CTL0 &= ~ENC;
    
    ADC10CTL1 = INCH_11;                     // AVcc/2
    ADC10CTL0 = SREF_1 + ADC10SHT_2 + REFON + ADC10ON + ADC10IE + REF2_5V;
    for( degC = 240; degC > 0; degC-- );    // delay to allow reference to settle
    ADC10CTL0 |= ENC + ADC10SC;             // Sampling and conversion start
    __bis_SR_register(CPUOFF + GIE);        // LPM0 with interrupts enabled
    results[1] = ADC10MEM;
    ADC10CTL0 &= ~ENC;
    ADC10CTL0 &= ~(REFON + ADC10ON);        // turn off A/D to save power
    
    // oC = ((A10/1024)*1500mV)-986mV)*1/3.55mV = A10*423/1024 - 278
    // the temperature is transmitted as an integer where 32.1 = 321
    // hence 4230 instead of 423
    temp = results[0];
    degC = ((temp - 673) * 4230) / 1024;
    if( tempOffset != 0xFFFF )
    {
      degC += tempOffset; 
    }
    /*message format,  UB = upper Byte, LB = lower Byte
    -------------------------------
    |degC LB | degC UB |  volt LB |
    -------------------------------
       0         1          2
    */
    
    temp = results[1];
    volt = (temp*25)/512;
    msg[0] = degC&0xFF;
    msg[1] = (degC>>8)&0xFF;
    msg[2] = volt;

    if (SMPL_SUCCESS == SMPL_Send(linkID1, msg, sizeof(msg)))
    {
      BSP_TOGGLE_LED2();
    }
    else
    {
      BSP_TOGGLE_LED2();
      BSP_TOGGLE_LED1();
    }
  }
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
  __bic_SR_register_on_exit(LPM3_bits);        // Clear LPM3 bit from 0(SR)
}
