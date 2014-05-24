/**************************************************************************************************
  Revised:        $Date: 2007-07-06 11:19:00 -0700 (Fri, 06 Jul 2007) $
  Revision:       $Revision: 13579 $

  Copyright 2007 Texas Instruments Incorporated.  All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights granted under
  the terms of a software license agreement between the user who downloaded the software,
  his/her employer (which must be your employer) and Texas Instruments Incorporated (the
  "License"). You may not use this Software unless you agree to abide by the terms of the
  License. The License limits your use, and you acknowledge, that the Software may not be
  modified, copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio frequency
  transceiver, which is integrated into your product. Other than for the foregoing purpose,
  you may not use, reproduce, copy, prepare derivative works of, modify, distribute,
  perform, display or sell this Software and/or its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE PROVIDED �AS IS�
  WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY
  WARRANTY OF MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE.
  IN NO EVENT SHALL TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER LEGAL EQUITABLE
  THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES INCLUDING BUT NOT LIMITED TO ANY
  INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST
  DATA, COST OF PROCUREMENT OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY
  THIRD PARTIES (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
**************************************************************************************************/

/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 *   BSP (Board Support Package)
 *   Target : Texas Instruments EZ430-RF2500
 *            "MSP430 Wireless Development Tool"
 *   Top-level board code file.
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 */

/* ------------------------------------------------------------------------------------------------
 *                                            Includes
 * ------------------------------------------------------------------------------------------------
 */
#include "bsp.h"
#include "bsp_config.h"

/* ------------------------------------------------------------------------------------------------
 *                                            Defines
 * ------------------------------------------------------------------------------------------------
 */
#define BSP_TIMER_CLK_MHZ   (BSP_CONFIG_CLOCK_MHZ_SELECT)
#define BSP_DELAY_MAX_USEC  (0xFFFF/BSP_TIMER_CLK_MHZ)

/**************************************************************************************************
 * @fn          BSP_InitBoard
 *
 * @brief       Initialize the board.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void BSP_InitBoard(void)
{
  /* disable watchdog timer */
  WDTCTL = WDTPW | WDTHOLD;

  /* configure internal digitally controlled oscillator */
  DCOCTL  = BSP_CONFIG_MSP430_DCOCTL;
  BCSCTL1 = BSP_CONFIG_MSP430_BCSCTL1;

  /* Configure TimerA for use by the delay function */
  /* Reset the timer */
  TACTL |= TACLR; /* Set the TACLR */
  /* Clear all settings */
  TACTL = 0x0;
  /* Select the clk source to be - SMCLK (Sub-Main CLK)*/
  TACTL |= TASSEL_2;
}

void TXString(char* string, int length)
{
  int pointer;
  for( pointer = 0; pointer < length; pointer++)
  {
    volatile int i;
    UCA0TXBUF = string[pointer];
    while (!(IFG2&UCA0TXIFG));              // USCI_A0 TX buffer ready?
  }
}

/**************************************************************************************************
 * @fn          BSP_Delay
 *
 * @brief       Sleep for the requested amount of time.
 *
 * @param       # of microseconds to sleep.
 *
 * @return      none
 **************************************************************************************************
 */
void BSP_Delay(uint32_t usec)
{
  BSP_ASSERT(usec < BSP_DELAY_MAX_USEC);

  TAR = 0; /* initial count */
  TACCR0 = BSP_TIMER_CLK_MHZ*usec; /* compare count. (delay in ticks) */

  /* Start the timer in UP mode */
  TACTL |= MC_1;

  /* Loop till compare interrupt flag is set */
  while(!(TACCTL0 & CCIFG));

  /* Stop the timer */
  TACTL &= ~(MC_1);

  /* Clear the interrupt flag */
   TACCTL0 &= ~CCIFG;
}

/**************************************************************************************************
*/



