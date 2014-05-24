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
 *   SPI interface code.
 * ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
 */

#ifndef MRFI_CC2500_SPI_H
#define MRFI_CC2500_SPI_H

/* ------------------------------------------------------------------------------------------------
 *                                         Includes
 * ------------------------------------------------------------------------------------------------
 */
#include "bsp.h"


/* ------------------------------------------------------------------------------------------------
 *                                          Defines
 * ------------------------------------------------------------------------------------------------
 */

/* configuration registers */
#define MRFI_CC2500_SPI_REG_IOCFG2       0x00      /*  IOCFG2   - GDO2 output pin configuration  */
#define MRFI_CC2500_SPI_REG_IOCFG1       0x01      /*  IOCFG1   - GDO1 output pin configuration  */
#define MRFI_CC2500_SPI_REG_IOCFG0       0x02      /*  IOCFG1   - GDO0 output pin configuration  */
#define MRFI_CC2500_SPI_REG_FIFOTHR      0x03      /*  FIFOTHR  - RX FIFO and TX FIFO thresholds */
#define MRFI_CC2500_SPI_REG_SYNC1        0x04      /*  SYNC1    - Sync word, high byte */
#define MRFI_CC2500_SPI_REG_SYNC0        0x05      /*  SYNC0    - Sync word, low byte */
#define MRFI_CC2500_SPI_REG_PKTLEN       0x06      /*  PKTLEN   - Packet length */
#define MRFI_CC2500_SPI_REG_PKTCTRL1     0x07      /*  PKTCTRL1 - Packet automation control */
#define MRFI_CC2500_SPI_REG_PKTCTRL0     0x08      /*  PKTCTRL0 - Packet automation control */
#define MRFI_CC2500_SPI_REG_ADDR         0x09      /*  ADDR     - Device address */
#define MRFI_CC2500_SPI_REG_CHANNR       0x0A      /*  CHANNR   - Channel number */
#define MRFI_CC2500_SPI_REG_FSCTRL1      0x0B      /*  FSCTRL1  - Frequency synthesizer control */
#define MRFI_CC2500_SPI_REG_FSCTRL0      0x0C      /*  FSCTRL0  - Frequency synthesizer control */
#define MRFI_CC2500_SPI_REG_FREQ2        0x0D      /*  FREQ2    - Frequency control word, high byte */
#define MRFI_CC2500_SPI_REG_FREQ1        0x0E      /*  FREQ1    - Frequency control word, middle byte */
#define MRFI_CC2500_SPI_REG_FREQ0        0x0F      /*  FREQ0    - Frequency control word, low byte */
#define MRFI_CC2500_SPI_REG_MDMCFG4      0x10      /*  MDMCFG4  - Modem configuration */
#define MRFI_CC2500_SPI_REG_MDMCFG3      0x11      /*  MDMCFG3  - Modem configuration */
#define MRFI_CC2500_SPI_REG_MDMCFG2      0x12      /*  MDMCFG2  - Modem configuration */
#define MRFI_CC2500_SPI_REG_MDMCFG1      0x13      /*  MDMCFG1  - Modem configuration */
#define MRFI_CC2500_SPI_REG_MDMCFG0      0x14      /*  MDMCFG0  - Modem configuration */
#define MRFI_CC2500_SPI_REG_DEVIATN      0x15      /*  DEVIATN  - Modem deviation setting */
#define MRFI_CC2500_SPI_REG_MCSM2        0x16      /*  MCSM2    - Main Radio Control State Machine configuration */
#define MRFI_CC2500_SPI_REG_MCSM1        0x17      /*  MCSM1    - Main Radio Control State Machine configuration */
#define MRFI_CC2500_SPI_REG_MCSM0        0x18      /*  MCSM0    - Main Radio Control State Machine configuration */
#define MRFI_CC2500_SPI_REG_FOCCFG       0x19      /*  FOCCFG   - Frequency Offset Compensation configuration */
#define MRFI_CC2500_SPI_REG_BSCFG        0x1A      /*  BSCFG    - Bit Synchronization configuration */
#define MRFI_CC2500_SPI_REG_AGCCTRL2     0x1B      /*  AGCCTRL2 - AGC control */
#define MRFI_CC2500_SPI_REG_AGCCTRL1     0x1C      /*  AGCCTRL1 - AGC control */
#define MRFI_CC2500_SPI_REG_AGCCTRL0     0x1D      /*  AGCCTRL0 - AGC control */
#define MRFI_CC2500_SPI_REG_WOREVT1      0x1E      /*  WOREVT1  - High byte Event0 timeout */
#define MRFI_CC2500_SPI_REG_WOREVT0      0x1F      /*  WOREVT0  - Low byte Event0 timeout */
#define MRFI_CC2500_SPI_REG_WORCTRL      0x20      /*  WORCTRL  - Wake On Radio control */
#define MRFI_CC2500_SPI_REG_FREND1       0x21      /*  FREND1   - Front end RX configuration */
#define MRFI_CC2500_SPI_REG_FREND0       0x22      /*  FREDN0   - Front end TX configuration */
#define MRFI_CC2500_SPI_REG_FSCAL3       0x23      /*  FSCAL3   - Frequency synthesizer calibration */
#define MRFI_CC2500_SPI_REG_FSCAL2       0x24      /*  FSCAL2   - Frequency synthesizer calibration */
#define MRFI_CC2500_SPI_REG_FSCAL1       0x25      /*  FSCAL1   - Frequency synthesizer calibration */
#define MRFI_CC2500_SPI_REG_FSCAL0       0x26      /*  FSCAL0   - Frequency synthesizer calibration */
#define MRFI_CC2500_SPI_REG_RCCTRL1      0x27      /*  RCCTRL1  - RC oscillator configuration */
#define MRFI_CC2500_SPI_REG_RCCTRL0      0x28      /*  RCCTRL0  - RC oscillator configuration */
#define MRFI_CC2500_SPI_REG_FSTEST       0x29      /*  FSTEST   - Frequency synthesizer calibration control */
#define MRFI_CC2500_SPI_REG_PTEST        0x2A      /*  PTEST    - Production test */
#define MRFI_CC2500_SPI_REG_AGCTEST      0x2B      /*  AGCTEST  - AGC test */
#define MRFI_CC2500_SPI_REG_TEST2        0x2C      /*  TEST2    - Various test settings */
#define MRFI_CC2500_SPI_REG_TEST1        0x2D      /*  TEST1    - Various test settings */
#define MRFI_CC2500_SPI_REG_TEST0        0x2E      /*  TEST0    - Various test settings */

/* status registers */
#define MRFI_CC2500_SPI_REG_PARTNUM      0x30      /*  PARTNUM    - Chip ID */
#define MRFI_CC2500_SPI_REG_VERSION      0x31      /*  VERSION    - Chip ID */
#define MRFI_CC2500_SPI_REG_FREQEST      0x32      /*  FREQEST    – Frequency Offset Estimate from demodulator */
#define MRFI_CC2500_SPI_REG_LQI          0x33      /*  LQI        – Demodulator estimate for Link Quality */
#define MRFI_CC2500_SPI_REG_RSSI         0x34      /*  RSSI       – Received signal strength indication */
#define MRFI_CC2500_SPI_REG_MARCSTATE    0x35      /*  MARCSTATE  – Main Radio Control State Machine state */
#define MRFI_CC2500_SPI_REG_WORTIME1     0x36      /*  WORTIME1   – High byte of WOR time */
#define MRFI_CC2500_SPI_REG_WORTIME0     0x37      /*  WORTIME0   – Low byte of WOR time */
#define MRFI_CC2500_SPI_REG_PKTSTATUS    0x38      /*  PKTSTATUS  – Current GDOx status and packet status */
#define MRFI_CC2500_SPI_REG_VCO_VC_DAC   0x39      /*  VCO_VC_DAC – Current setting from PLL calibration module */
#define MRFI_CC2500_SPI_REG_TXBYTES      0x3A      /*  TXBYTES    – Underflow and number of bytes */
#define MRFI_CC2500_SPI_REG_RXBYTES      0x3B      /*  RXBYTES    – Overflow and number of bytes */
                                                                                                                                                 
/* burst write registers */
#define MRFI_CC2500_SPI_REG_PATABLE      0x3E      /*  PATABLE - PA control settings table */
#define MRFI_CC2500_SPI_REG_TXFIFO       0x3F      /*  TXFIFO  - Transmit FIFO */
#define MRFI_CC2500_SPI_REG_RXFIFO       0x3F      /*  RXFIFO  - Receive FIFO */

/* command strobe registers */
#define MRFI_CC2500_SPI_STROBE_SRES      0x30      /*  SRES    - Reset chip. */
#define MRFI_CC2500_SPI_STROBE_SFSTXON   0x31      /*  SFSTXON - Enable and calibrate frequency synthesizer. */
#define MRFI_CC2500_SPI_STROBE_SXOFF     0x32      /*  SXOFF   - Turn off crystal oscillator. */
#define MRFI_CC2500_SPI_STROBE_SCAL      0x33      /*  SCAL    - Calibrate frequency synthesizer and turn it off. */
#define MRFI_CC2500_SPI_STROBE_SRX       0x34      /*  SRX     - Enable RX. Perform calibration if enabled. */
#define MRFI_CC2500_SPI_STROBE_STX       0x35      /*  STX     - Enable TX. If in RX state, only enable TX if CCA passes. */
#define MRFI_CC2500_SPI_STROBE_SIDLE     0x36      /*  SIDLE   - Exit RX / TX, turn off frequency synthesizer. */
#define MRFI_CC2500_SPI_STROBE_SRSVD     0x37      /*  SRVSD   - Reserved.  Do not use. */
#define MRFI_CC2500_SPI_STROBE_SWOR      0x38      /*  SWOR    - Start automatic RX polling sequence (Wake-on-Radio) */
#define MRFI_CC2500_SPI_STROBE_SPWD      0x39      /*  SPWD    - Enter power down mode when CSn goes high. */
#define MRFI_CC2500_SPI_STROBE_SFRX      0x3A      /*  SFRX    - Flush the RX FIFO buffer. */
#define MRFI_CC2500_SPI_STROBE_SFTX      0x3B      /*  SFTX    - Flush the TX FIFO buffer. */
#define MRFI_CC2500_SPI_STROBE_SWORRST   0x3C      /*  SWORRST - Reset real time clock. */
#define MRFI_CC2500_SPI_STROBE_SNOP      0x3D      /*  SNOP    - No operation. Returns status byte. */


/* ------------------------------------------------------------------------------------------------
 *                                         Prototypes
 * ------------------------------------------------------------------------------------------------
 */
void mrfiSpiInit(void);

uint8_t mrfiSpiCmdStrobe(uint8_t addr);

uint8_t mrfiSpiReadReg(uint8_t addr);
void mrfiSpiWriteReg(uint8_t addr, uint8_t value);

void mrfiSpiWriteTxFifo(uint8_t * pWriteData, uint8_t len);
void mrfiSpiReadRxFifo(uint8_t * pReadData, uint8_t len);


/**************************************************************************************************
 */
#endif
