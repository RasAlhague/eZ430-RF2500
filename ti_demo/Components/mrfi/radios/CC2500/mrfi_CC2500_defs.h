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

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE PROVIDED “AS IS”
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

/* ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
 *   MRFI (Minimal RF Interface)
 *   Radios: CC2500, CC1100, CC1101
 *   Radio definition file for supported radios.
 * ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
 */

#ifndef MRFI_CC2500_DEFS_H
#define MRFI_CC2500_DEFS_H


/* ------------------------------------------------------------------------------------------------
 *                                          Defines
 * ------------------------------------------------------------------------------------------------
 */


#define __mrfi_LENGTH_FIELD_SIZE__  1
#define __mrfi_ADDR_SIZE__          4
#define __mrfi_MAX_PAYLOAD__        20
#define __mrfi_RX_METRICS_SIZE__    2

#define __mrfi_LENGTH_FIELD_OFS__   0
#define __mrfi_DST_ADDR_OFS__       (__mrfi_LENGTH_FIELD_OFS__ + __mrfi_LENGTH_FIELD_SIZE__)
#define __mrfi_SRC_ADDR_OFS__       (__mrfi_DST_ADDR_OFS__ + __mrfi_ADDR_SIZE__)
#define __mrfi_PAYLOAD_OFS__        (__mrfi_SRC_ADDR_OFS__ + __mrfi_ADDR_SIZE__)

#define __mrfi_HEADER_SIZE__        (2 * __mrfi_ADDR_SIZE__)
#define __mrfi_MAX_FRAME_SIZE__     (__mrfi_LENGTH_FIELD_SIZE__ + __mrfi_HEADER_SIZE__ + __mrfi_MAX_PAYLOAD__)

/* if external code has defined a maximum payload, use that instead of default */
#ifdef MAX_APP_PAYLOAD
#undef __mrfi_MAX_PAYLOAD__
#define __mrfi_MAX_PAYLOAD__  MAX_APP_PAYLOAD+3 /* SimpliciTI payload size plus three byte overhead */
#endif


/* ------------------------------------------------------------------------------------------------
 *                                          Macros
 * ------------------------------------------------------------------------------------------------
 */
#define __mrfi_GET_PAYLOAD_LEN__(p)       ((p)->frame[__mrfi_LENGTH_FIELD_OFS__] - __mrfi_HEADER_SIZE__)
#define __mrfi_SET_PAYLOAD_LEN__(p,x)     st( (p)->frame[__mrfi_LENGTH_FIELD_OFS__] = x + __mrfi_HEADER_SIZE__; )

#define __mrfi_P_DST_ADDR__(p)            (&((p)->frame[__mrfi_DST_ADDR_OFS__]))
#define __mrfi_P_SRC_ADDR__(p)            (&((p)->frame[__mrfi_SRC_ADDR_OFS__]))
#define __mrfi_P_PAYLOAD__(p)             (&((p)->frame[__mrfi_PAYLOAD_OFS__]))


/* ------------------------------------------------------------------------------------------------
 *                                          Typedefs
 * ------------------------------------------------------------------------------------------------
 */
typedef   unsigned char   __mrfi_OutputPower_t__;


/* ------------------------------------------------------------------------------------------------
 *                                       Radio Abstraction
 * ------------------------------------------------------------------------------------------------
 */
#if (defined MRFI_CC1100)
#define MRFI_SMARTRF_SETTINGS_H   "radios/CC1100/smartrf_CC1100.h"
#define MRFI_SETTING_PATABLE0     0x51
#define MRFI_RADIO_PARTNUM        0x00
#define MRFI_RADIO_MIN_VERSION    3

#elif (defined MRFI_CC1101)
#define MRFI_SMARTRF_SETTINGS_H   "radios/CC1101/smartrf_CC1101.h"
#define MRFI_SETTING_PATABLE0     0x51
#define MRFI_RADIO_PARTNUM        0x00
#define MRFI_RADIO_MIN_VERSION    4

#elif (defined MRFI_CC2500)
#define MRFI_SMARTRF_SETTINGS_H   "radios/CC2500/smartrf_CC2500.h"
#define MRFI_SETTING_PATABLE0     0xFE
#define MRFI_RADIO_PARTNUM        0x80
#define MRFI_RADIO_MIN_VERSION    3

#else
#error "ERROR: Unspecified or unsupported radio."
#endif


/**************************************************************************************************
 */
#endif
