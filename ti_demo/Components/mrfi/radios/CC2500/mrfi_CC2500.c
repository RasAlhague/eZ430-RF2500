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
 *   Primary code file for supported radios.
 * ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
 */

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */
#include <string.h>

#include "mrfi.h"
#include "bsp.h"
#include "bsp_macros.h"
#include "mrfi_CC2500_spi.h"
#include "mrfi_CC2500_defs.h"

/* must include after inclusion of mrfi_CC2500_defs.h */
#include MRFI_SMARTRF_SETTINGS_H


/* ------------------------------------------------------------------------------------------------
 *                                          Defines
 * ------------------------------------------------------------------------------------------------
 */
#define MRFI_CCA_RETRIES                    4

#define MRFI_LENGTH_FIELD_OFS               __mrfi_LENGTH_FIELD_OFS__
#define MRFI_LENGTH_FIELD_SIZE              __mrfi_LENGTH_FIELD_SIZE__
#define MRFI_HEADER_SIZE                    __mrfi_HEADER_SIZE__
#define MRFI_FRAME_BODY_OFS                 __mrfi_DST_ADDR_OFS__

/* radio states from status byte */
#define MRFI_RADIO_STATE_MASK               0x70
#define MRFI_RADIO_STATE_IDLE               0x00
#define MRFI_RADIO_STATE_RX                 0x10
#define MRFI_RADIO_STATE_TX                 0x20
#define MRFI_RADIO_STATE_FSTXON             0x30
#define MRFI_RADIO_STATE_CALIBRATE          0x40
#define MRFI_RADIO_STATE_SETTLING           0x50
#define MRFI_RADIO_STATE_RXFIFO_OVERFLOW    0x60
#define MRFI_RADIO_STATE_TXFIFO_UNDERFLOW   0x70

/* rx metrics definitions, known as appended "packet status bytes" in datasheet parlance */
#define MRFI_RX_METRICS_RSSI_OFS            0
#define MRFI_RX_METRICS_CRC_LQI_OFS         1
#define MRFI_RX_METRICS_CRC_OK_MASK         0x80
#define MRFI_RX_METRICS_LQI_MASK            0x7F

/* GDO functionality */
#define MRFI_GDO_SYNC           6
#define MRFI_GDO_CCA            9
#define MRFI_GDO_PA_PD          27  /* low when transmit is active, low during sleep */
#define MRFI_GDO_LNA_PD         28  /* low when receive is active, low during sleep */

/* GDO0 output pin configuration */
#define MRFI_SETTING_IOCFG0     MRFI_GDO_SYNC

/* GDO2 output pin configuration */
#define MRFI_SETTING_IOCFG2     MRFI_GDO_PA_PD

/* Main Radio Control State Machine control configuration - Original value except PO_TIMEOUT is extracted from SmartRF setting. */
#define MRFI_SETTING_MCSM0      (0x10 | (SMARTRF_SETTING_MCSM0 & (BV(2)|BV(3))))

/* Main Radio Control State Machine control configuration - Go to RX state after both RX and TX. */
#define MRFI_SETTING_MCSM1      0x3F    

/*
 *  Packet Length - Setting for maximum allowed packet length.
 *  The PKTLEN setting does not include the length field but maximum frame size does.
 *  Subtract length filed from maximum frame size to get value for PKTLEN.
 */
#define MRFI_SETTING_PKTLEN     (MRFI_MAX_FRAME_SIZE - MRFI_LENGTH_FIELD_SIZE)

/* Packet automation control - Original value except WHITE_DATA is extracted from SmartRF setting. */
#define MRFI_SETTING_PKTCTRL0   (0x05 | (SMARTRF_SETTING_PKTCTRL0 & BV(6)))

/* Packet automation control - base value is power up value whick has APPEND_STATUS enabled; no CRC autoflush */
#define MRFI_SETTING_PKTCTRL1_BASE              BV(2)
#define MRFI_SETTING_PKTCTRL1_ADDR_FILTER_OFF   MRFI_SETTING_PKTCTRL1_BASE
#define MRFI_SETTING_PKTCTRL1_ADDR_FILTER_ON    (MRFI_SETTING_PKTCTRL1_BASE | 0x01)

/* FIFO threshold - this register has fields that need to be configured for the CC1101 */
#define MRFI_SETTING_FIFOTHR    (0x07 | (SMARTRF_SETTING_FIFOTHR & (BV(4)|BV(5)|BV(6))))

/* if SmartRF setting for PATABLE[0] is supplied, use that instead of built-in default */
#ifdef SMARTRF_SETTING_PATABLE0
#undef MRFI_SETTING_PATABLE0
#define MRFI_SETTING_PATABLE0   SMARTRF_SETTING_PATABLE0
#endif

/* channel is taken from the SmartRF settings unless an override is supplied */
#ifdef MRFI_CHAN
#define MRFI_SETTING_CHANNR   MRFI_CHAN
#else
#define MRFI_SETTING_CHANNR   SMARTRF_SETTING_CHANNR
#endif


/* ------------------------------------------------------------------------------------------------
 *                                           Macros
 * ------------------------------------------------------------------------------------------------
 */

/* for readability, alias the GDO0 macros to more meaningful names */
#define MRFI_SYNC_PIN_IS_HIGH()                     MRFI_GDO0_PIN_IS_HIGH()
#define MRFI_ENABLE_SYNC_PIN_INT()                  MRFI_ENABLE_GDO0_INT()
#define MRFI_DISABLE_SYNC_PIN_INT()                 MRFI_DISABLE_GDO0_INT()
#define MRFI_SYNC_PIN_INT_IS_ENABLED()              MRFI_GDO0_INT_IS_ENABLED()
#define MRFI_CLEAR_SYNC_PIN_INT_FLAG()              MRFI_CLEAR_GDO0_INT_FLAG()
#define MRFI_SYNC_PIN_INT_FLAG_IS_SET()             MRFI_GDO0_INT_FLAG_IS_SET()
#define MRFI_CONFIG_SYNC_PIN_RISING_EDGE_INT()      MRFI_CONFIG_GDO0_RISING_EDGE_INT()
#define MRFI_CONFIG_SYNC_PIN_FALLING_EDGE_INT()     MRFI_CONFIG_GDO0_FALLING_EDGE_INT()

/* for readability, alias the GDO2 macros to more meaningful names */
#define MRFI_PAPD_PIN_IS_HIGH()                     MRFI_GDO2_PIN_IS_HIGH()
#define MRFI_CLEAR_PAPD_PIN_INT_FLAG()              MRFI_CLEAR_GDO2_INT_FLAG()
#define MRFI_PAPD_INT_FLAG_IS_SET()                 MRFI_GDO2_INT_FLAG_IS_SET()
#define MRFI_CONFIG_PAPD_RISING_EDGE_INT()          MRFI_CONFIG_GDO2_RISING_EDGE_INT()
#define MRFI_CONFIG_PAPD_FALLING_EDGE_INT()         MRFI_CONFIG_GDO2_FALLING_EDGE_INT()

#define MRFI_GET_RADIO_STATE()          (mrfiSpiCmdStrobe(MRFI_CC2500_SPI_STROBE_SNOP) & MRFI_RADIO_STATE_MASK)

#define MRFI_ASSERT(x)                  BSP_ASSERT(x)
#define MRFI_ASSERTS_ARE_ON             BSP_ASSERTS_ARE_ON

/*
 *  This macro delays an amount of time given in arbitrary units.  Obviously the timing
 *  is dependent on compiler efficiency and clock speed.  This should be cleaned up.
 */
#define MRFI_DELAY(x)                   st( volatile uint16_t i = x; while(i--); )


/* ------------------------------------------------------------------------------------------------
 *                                          Constants
 * ------------------------------------------------------------------------------------------------
 */
const uint8_t mrfiRadioCfg[][2] =
{
  /* internal radio configuration */
  { MRFI_CC2500_SPI_REG_IOCFG0,    MRFI_SETTING_IOCFG0   },
  { MRFI_CC2500_SPI_REG_IOCFG2,    MRFI_SETTING_IOCFG2   },
  { MRFI_CC2500_SPI_REG_MCSM1,     MRFI_SETTING_MCSM1    },
  { MRFI_CC2500_SPI_REG_MCSM0,     MRFI_SETTING_MCSM0    },
  { MRFI_CC2500_SPI_REG_PKTLEN,    MRFI_SETTING_PKTLEN   },
  { MRFI_CC2500_SPI_REG_PKTCTRL0,  MRFI_SETTING_PKTCTRL0 }, 
  { MRFI_CC2500_SPI_REG_PATABLE,   MRFI_SETTING_PATABLE0 }, 
  { MRFI_CC2500_SPI_REG_CHANNR,    MRFI_SETTING_CHANNR   },
#ifdef MRFI_CC1101
  { MRFI_CC2500_SPI_REG_FIFOTHR,   MRFI_SETTING_FIFOTHR  },
#endif

  /* imported SmartRF radio configuration */
  { MRFI_CC2500_SPI_REG_FSCTRL1,   SMARTRF_SETTING_FSCTRL1  },
  { MRFI_CC2500_SPI_REG_FSCTRL0,   SMARTRF_SETTING_FSCTRL0  },
  { MRFI_CC2500_SPI_REG_FREQ2,     SMARTRF_SETTING_FREQ2    },
  { MRFI_CC2500_SPI_REG_FREQ1,     SMARTRF_SETTING_FREQ1    },
  { MRFI_CC2500_SPI_REG_FREQ0,     SMARTRF_SETTING_FREQ0    },
  { MRFI_CC2500_SPI_REG_MDMCFG4,   SMARTRF_SETTING_MDMCFG4  },
  { MRFI_CC2500_SPI_REG_MDMCFG3,   SMARTRF_SETTING_MDMCFG3  },
  { MRFI_CC2500_SPI_REG_MDMCFG2,   SMARTRF_SETTING_MDMCFG2  },
  { MRFI_CC2500_SPI_REG_MDMCFG1,   SMARTRF_SETTING_MDMCFG1  },
  { MRFI_CC2500_SPI_REG_MDMCFG0,   SMARTRF_SETTING_MDMCFG0  },
  { MRFI_CC2500_SPI_REG_DEVIATN,   SMARTRF_SETTING_DEVIATN  },
  { MRFI_CC2500_SPI_REG_FOCCFG,    SMARTRF_SETTING_FOCCFG   },
  { MRFI_CC2500_SPI_REG_BSCFG,     SMARTRF_SETTING_BSCFG    },
  { MRFI_CC2500_SPI_REG_AGCCTRL2,  SMARTRF_SETTING_AGCCTRL2 },
  { MRFI_CC2500_SPI_REG_AGCCTRL1,  SMARTRF_SETTING_AGCCTRL1 },
  { MRFI_CC2500_SPI_REG_AGCCTRL0,  SMARTRF_SETTING_AGCCTRL0 },
  { MRFI_CC2500_SPI_REG_FREND1,    SMARTRF_SETTING_FREND1   },
  { MRFI_CC2500_SPI_REG_FREND0,    SMARTRF_SETTING_FREND0   },
  { MRFI_CC2500_SPI_REG_FSCAL3,    SMARTRF_SETTING_FSCAL3   },
  { MRFI_CC2500_SPI_REG_FSCAL2,    SMARTRF_SETTING_FSCAL2   },
  { MRFI_CC2500_SPI_REG_FSCAL1,    SMARTRF_SETTING_FSCAL1   },
  { MRFI_CC2500_SPI_REG_FSCAL0,    SMARTRF_SETTING_FSCAL0   },
  { MRFI_CC2500_SPI_REG_TEST2,     SMARTRF_SETTING_TEST2    },
  { MRFI_CC2500_SPI_REG_TEST1,     SMARTRF_SETTING_TEST1    },
  { MRFI_CC2500_SPI_REG_TEST0,     SMARTRF_SETTING_TEST0    },
};

/*
 *  This assert happens if configuration has extraneous compiler padding.
 *  Modify compiler settings for no padding, or, if that is not possible,
 *  comment out this assert.
 */
BSP_STATIC_ASSERT(sizeof(mrfiRadioCfg) == ((sizeof(mrfiRadioCfg)/sizeof(mrfiRadioCfg[0])) * sizeof(mrfiRadioCfg[0])));


/* ------------------------------------------------------------------------------------------------
 *                                       Local Prototypes
 * ------------------------------------------------------------------------------------------------
 */
static void MRFI_SyncPinRxIsr(void);


/* ------------------------------------------------------------------------------------------------
 *                                       Local Variables
 * ------------------------------------------------------------------------------------------------
 */
static uint8_t mrfiRxActive;
static uint8_t mrfiTxActive;
static uint8_t mrfiRadioIsSleeping;
static uint8_t mrfiRxFilterEnabled;
static uint8_t mrfiRxFilterAddr[MRFI_ADDR_SIZE];
static mrfiPacket_t mrfiIncomingPacket;


/**************************************************************************************************
 * @fn          MRFI_Init
 *
 * @brief       Initialize MRFI.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void MRFI_Init(void)
{
  /* ------------------------------------------------------------------
   *    Initialization
   *   -----------------
   */

  /* initialize radio state variables */
  mrfiRxFilterEnabled = 0;
  mrfiRadioIsSleeping = 0;
  mrfiTxActive = 0;
  mrfiRxActive = 0;
  
  /* initialize GPIO pins */
  MRFI_CONFIG_GDO0_PIN_AS_INPUT();
  MRFI_CONFIG_GDO2_PIN_AS_INPUT();
  
  /* initialize SPI */
  mrfiSpiInit();
  
  /* ------------------------------------------------------------------
   *    Radio power-up reset
   *   ----------------------
   */
  MRFI_ASSERT(MRFI_SPI_CSN_IS_HIGH());
  
  /* pulse CSn low then high */
  MRFI_SPI_DRIVE_CSN_LOW();
  MRFI_DELAY(10);
  MRFI_SPI_DRIVE_CSN_HIGH();

  /* hold CSn high for at least 40 microseconds */
  MRFI_DELAY(100);
  
  /* pull CSn low and wait for SO to go low */
  MRFI_SPI_DRIVE_CSN_LOW();
  while (MRFI_SPI_SO_IS_HIGH());

  /* directly send strobe command - cannot use function as it affects CSn pin */
  MRFI_SPI_WRITE_BYTE(MRFI_CC2500_SPI_STROBE_SRES);
  MRFI_SPI_WAIT_DONE();

  /* wait for SO to go low again, reset is complete at that point */
  while (MRFI_SPI_SO_IS_HIGH());

  /* return CSn pin to its default high level */
  MRFI_SPI_DRIVE_CSN_HIGH();

  /* ------------------------------------------------------------------
   *    Run-time integrity checks
   *   ---------------------------
   */
  
  /* verify that SPI is working */
#ifdef MRFI_ASSERTS_ARE_ON
#define TEST_VALUE 0xA5
  mrfiSpiWriteReg( MRFI_CC2500_SPI_REG_PKTLEN, TEST_VALUE );
  MRFI_ASSERT( mrfiSpiReadReg( MRFI_CC2500_SPI_REG_PKTLEN ) == TEST_VALUE ); /* SPI is not responding */
#endif

  /* verify the correct radio is installed */
  MRFI_ASSERT( mrfiSpiReadReg( MRFI_CC2500_SPI_REG_PARTNUM ) == MRFI_RADIO_PARTNUM);      /* incorrect radio specified */
  MRFI_ASSERT( mrfiSpiReadReg( MRFI_CC2500_SPI_REG_VERSION ) >= MRFI_RADIO_MIN_VERSION);  /* obsolete radio specified  */
  
  /* ------------------------------------------------------------------
   *    Configure radio
   *   -----------------
   */

  /* initialize radio registers */
  {
    uint8_t i;
    
    for (i=0; i<(sizeof(mrfiRadioCfg)/sizeof(mrfiRadioCfg[0])); i++)
    {
      mrfiSpiWriteReg(mrfiRadioCfg[i][0], mrfiRadioCfg[i][1]);
    }
  }

  /* send strobe to turn on receiver */
  mrfiSpiCmdStrobe(MRFI_CC2500_SPI_STROBE_SRX);
  
  /* ------------------------------------------------------------------
   *    Configure interrupts
   *   ----------------------
   */

  /*
   *  Configure and enable the SYNC signal interrupt.
   *
   *  This interrupt is used to indicate receive.  The SYNC signal goes
   *  high when a receive OR a transmit begins.  It goes high once the
   *  sync word is received or transmitted and then goes low again once
   *  the packet completes.
   */
  MRFI_CONFIG_SYNC_PIN_FALLING_EDGE_INT();
  MRFI_CLEAR_SYNC_PIN_INT_FLAG();
  MRFI_ENABLE_SYNC_PIN_INT();
  
  /* configure PA_PD signal interrupt */
  MRFI_CONFIG_PAPD_FALLING_EDGE_INT();
  
  /* enable global interrupts */
  BSP_ENABLE_INTERRUPTS();
}


/**************************************************************************************************
 * @fn          MRFI_Transmit
 *
 * @brief       Transmit a packet using CCA algorithm.
 *
 * @param       pPacket - pointer to packet to transmit
 *
 * @return      Return code indicates success or failure of transmit:
 *                  MRFI_TRANSMIT_SUCCESS - transmit succeeded
 *                  MRFI_TRANSMIT_CCA_FAILED - transmit failed because of CCA check(s)
 *                  MRFI_TRANSMIT_RADIO_ASLEEP - transmit failed because radio was asleep
 **************************************************************************************************
 */
uint8_t MRFI_Transmit(mrfiPacket_t * pPacket)
{
  uint8_t ccaRetries;
  uint8_t txBufLen;
  uint8_t savedSyncIntState;
  uint8_t returnValue;

  /* abort transmit if radio is asleep */
  {
    bspIState_t s;

    /* critical section necessary for watertight testing and setting of state variables */
    BSP_ENTER_CRITICAL_SECTION(s);

    /* if radio is asleep, abort transmit and return reason for failure */
    if (mrfiRadioIsSleeping)
    {
      BSP_EXIT_CRITICAL_SECTION(s);
      return( MRFI_TRANSMIT_RADIO_ASLEEP );
    }
    
    /* radio is not asleep, set flag that indicates transmit is active */
    mrfiTxActive = 1;
    BSP_EXIT_CRITICAL_SECTION(s);
  }
  
  /* set number of CCA retries */
  ccaRetries = MRFI_CCA_RETRIES;
  
  /* compute number of bytes to write to transmit FIFO */
  txBufLen = pPacket->frame[MRFI_LENGTH_FIELD_OFS] + MRFI_LENGTH_FIELD_SIZE;
  
  /* write packet to transmit FIFO */
  mrfiSpiWriteTxFifo(&(pPacket->frame[0]), txBufLen);
  
  /* ===============================================================================
   *    Main Loop 
   *  =============
   */
  for (;;)
  {
    /* CCA delay */
    MRFI_DELAY(2000);

    /* disable sync pin interrupts, necessary because both transmit and receive affect sync signal */
    MRFI_DISABLE_SYNC_PIN_INT();

    /* store the sync pin interrupt flag, important so original state can be restored */
    savedSyncIntState = MRFI_SYNC_PIN_INT_FLAG_IS_SET();
      
    /*
     *  Clear the PA_PD pin interrupt flag.  This flag, not the interrupt itself,
     *  is used to capture the transition that indicates a transmit was started.
     *  The pin level cannot be used to indicate transmit success as timing may
     *  prevent the transition from being detected.  The interrupt latch captures
     *  the event regardless of timing.
     */
    MRFI_CLEAR_PAPD_PIN_INT_FLAG();
    
    /* send strobe to initiate transmit */
    mrfiSpiCmdStrobe(MRFI_CC2500_SPI_STROBE_STX);
    
    /*  delay long enough for the PA_PD signal to indicate a successful transmit */
    MRFI_DELAY(250);

    /* if the interrupt flag of the PA_PD pin is set, CCA passed and the transmit has started */
    if (MRFI_PAPD_INT_FLAG_IS_SET())
    {
      /* ------------------------------------------------------------------
       *    Clear Channel Assessment passed.
       *   ----------------------------------
       */

      /* wait for transmit to complete */
      while (!MRFI_PAPD_PIN_IS_HIGH());

      /*
       *  Restore the original sync interrupt state.  The successful transmit just
       *  caused a transition on the sync signal that set the flag (if not already
       *  set).  To restore the original state, the flag is simply cleared if it
       *  was clear earlier.
       */
      if (!savedSyncIntState)
      {
        MRFI_CLEAR_SYNC_PIN_INT_FLAG();
      }
      
      /* transmit complete, enable sync pin interrupts */
      MRFI_ENABLE_SYNC_PIN_INT();

      /* set return value for successful transmit and break */      
      returnValue = MRFI_TRANSMIT_SUCCESS;
      break;
    }

    /* ------------------------------------------------------------------
     *    Clear Channel Assessment failed.
     *   ----------------------------------
     */
    
    /* CCA failed, safe to enable sync pin interrupts */
    MRFI_ENABLE_SYNC_PIN_INT();

    /* if no CCA retries are left, transmit failed so abort */    
    if (ccaRetries == 0)
    {
      bspIState_t s;

      /*
       *  Flush the transmit FIFO.  It must be flushed so that
       *  the next transmit can start with a clean slate.
       *  Critical section prevents receive interrupt from
       *  occurring in the middle of clean-up.
       */
      BSP_ENTER_CRITICAL_SECTION(s);
      mrfiSpiCmdStrobe(MRFI_CC2500_SPI_STROBE_SIDLE);
      mrfiSpiCmdStrobe(MRFI_CC2500_SPI_STROBE_SFTX);
      mrfiSpiCmdStrobe(MRFI_CC2500_SPI_STROBE_SRX);
      BSP_EXIT_CRITICAL_SECTION(s);

      /* set return value for failed transmit and break */      
      returnValue = MRFI_TRANSMIT_CCA_FAILED;
      break;
    }

    /* decrement CCA retries before loop continues */
    ccaRetries--;
  }
  /*
   * =============================================================================== */

  mrfiTxActive = 0;
  return( returnValue );
}


/**************************************************************************************************
 * @fn          MRFI_Receive
 *
 * @brief       Copies last packet received to the location specified.
 *              This function is meant to be called after the ISR informs
 *              higher level code that there is a newly received packet.
 *
 * @param       pPacket - pointer to location of where to copy received packet
 *
 * @return      none
 **************************************************************************************************
 */
void MRFI_Receive(mrfiPacket_t * pPacket)
{
  *pPacket = mrfiIncomingPacket;
}


/**************************************************************************************************
 * @fn          MRFI_SyncPinRxIsr
 *
 * @brief       This interrupt is called when the SYNC signal transition from high to low. 
 *              The sync signal is routed to the sync pin which is a GPIO pin.  This high-to-low
 *              transition signifies a receive has completed.  The SYNC signal also goes from
 *              high to low when a transmit completes.   This is protected against within the
 *              transmit function by disabling sync pin interrupts until transmit completes.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
static void MRFI_SyncPinRxIsr(void)
{
  uint8_t frameLen;
  uint8_t rxBytes;

  /* ------------------------------------------------------------------
   *    Abort if asleep
   *   -----------------
   */
 
  /*
   *  If radio is asleep, abort immediately.  Nothing further is required.
   *  If radio is awake, set "receive active" flag and continue processing the receive.
   */
  {
    bspIState_t s;
    
    /* critical section necessary for watertight testing and setting of state variables */
    BSP_ENTER_CRITICAL_SECTION(s);

    /* if radio is asleep, just abort from here */
    if (mrfiRadioIsSleeping)
    {
      BSP_EXIT_CRITICAL_SECTION(s);
      return;
    }
    
    /* radio is not asleep, set flag that indicates receive is active */
    mrfiRxActive = 1;
    BSP_EXIT_CRITICAL_SECTION(s);
  }

  
  /* ------------------------------------------------------------------
   *    Get RXBYTES
   *   -------------
   */

  /*
   *  Read the RXBYTES register from the radio.
   *  Bit description of RXBYTES register:
   *    bit 7     - RXFIFO_OVERFLOW, set if receive overflow occurred
   *    bits 6:0  - NUM_BYTES, number of bytes in receive FIFO
   *
   *  Due a chip bug, the RXBYTES register must read the same value twice
   *  in a row to guarantee an accurate value.
   */
  {
    uint8_t rxBytesVerify;
    
    rxBytesVerify = mrfiSpiReadReg(MRFI_CC2500_SPI_REG_RXBYTES);

    do 
    {
      rxBytes = rxBytesVerify;
      rxBytesVerify = mrfiSpiReadReg(MRFI_CC2500_SPI_REG_RXBYTES);
    }
    while (rxBytes != rxBytesVerify);
  }

  
  /* ------------------------------------------------------------------
   *    FIFO empty?
   *   -------------
   */

  /*
   *  See if the receive FIFIO is empty before attempting to read from it.
   *  It is possible nothing the FIFO is empty even though the interrupt fired.
   *  This can happen if address check is enabled and a non-matching packet is
   *  received.  In that case, the radio automatically removes the packet from
   *  the FIFO.
   */
  if (rxBytes == 0)
  {
    /* receive FIFO is empty - do nothing, skip to end */
  }
  else
  {
    /* receive FIFO is not empty, continue processing */

    /* ------------------------------------------------------------------
     *    Process frame length
     *   ----------------------
     */
    
    /* read the first byte from FIFO - the packet length */
    mrfiSpiReadRxFifo(&frameLen, MRFI_LENGTH_FIELD_SIZE);

    /*
     *  Make sure that the frame length just read corresponds to number of bytes in the buffer.
     *  If these do not match up something is wrong.
     *
     *  This can happen for several reasons:
     *   1) Incoming packet has an incorrect format or is corrupted.
     *   2) The receive FIFO overflowed.  Overflow is indicated by the high
     *      bit of rxBytes.  This guarantees the value of rxBytes value will not
     *      match the number of bytes in the FIFO for overflow condition.
     *   3) Interrupts were blocked for an abnormally long time which
     *      allowed a following packet to at least start filling the
     *      receive FIFO.  In this case, all received and partially received
     *      packets will be lost - the packet in the FIFO and the packet coming in.
     *      This is the price the user pays if they implement a giant
     *      critical section.
     *   4) A failed transmit forced radio to IDLE state to flush the transmit FIFO.
     *      This could cause an active receive to be cut short.
     */
    if (rxBytes != (frameLen + MRFI_LENGTH_FIELD_SIZE + MRFI_RX_METRICS_SIZE))
    {
      bspIState_t s;

      /* mismatch between bytes-in-FIFO and frame length */

      /*
       *  Flush receive FIFO to reset receive.  Must go to IDLE state to do this.
       *  The critical section guarantees a transmit does not occur while cleaning up.
       */
      BSP_ENTER_CRITICAL_SECTION(s);
      mrfiSpiCmdStrobe(MRFI_CC2500_SPI_STROBE_SIDLE);
      mrfiSpiCmdStrobe(MRFI_CC2500_SPI_STROBE_SFRX);
      mrfiSpiCmdStrobe(MRFI_CC2500_SPI_STROBE_SRX);
      BSP_EXIT_CRITICAL_SECTION(s);
      
      /* flush complete, skip to end */
    }
    else
    {
      /* bytes-in-FIFO and frame length match up - continue processing */
      
      /* ------------------------------------------------------------------
       *    Get packet
       *   ------------
       */

      /* set length field */
      mrfiIncomingPacket.frame[MRFI_LENGTH_FIELD_OFS] = frameLen;
  
      /* get packet from FIFO */
      mrfiSpiReadRxFifo(&(mrfiIncomingPacket.frame[MRFI_FRAME_BODY_OFS]), frameLen);
  
      /* get receive metrics from FIFO */
      mrfiSpiReadRxFifo(&(mrfiIncomingPacket.rxMetrics[0]), MRFI_RX_METRICS_SIZE);


      /* ------------------------------------------------------------------
       *    CRC check
       *   ------------
       */

      /*
       *  Note!  Automatic CRC check is not, and must not, be enabled.  This feature
       *  flushes the *entire* receive FIFO when CRC fails.  If this feature is
       *  enabled it is possible to be reading from the FIFO and have a second
       *  receive occur that fails CRC and automatically flushes the receive FIFO.
       *  This could cause reads from an empty receive FIFO which puts the radio
       *  into an undefined state.
       */
      
      /* determine if CRC failed */
      if (!(mrfiIncomingPacket.rxMetrics[MRFI_RX_METRICS_CRC_LQI_OFS] & MRFI_RX_METRICS_CRC_OK_MASK))
      {
        /* CRC failed - do nothing, skip to end */
      }
      else
      {
        /* CRC passed - continue processing */

      /* ------------------------------------------------------------------
       *    Filtering 
       *   -----------
       */

        /* see if filtering is enabled and, if so, determine if address is a match */
        if (mrfiRxFilterEnabled && memcmp(MRFI_P_DST_ADDR(&mrfiIncomingPacket), &mrfiRxFilterAddr[0], MRFI_ADDR_SIZE))
        {
          /* packet filtered out - do nothing, skip to end */
        }
        else
        {
          /* packet not filtered out - receive successful */
          
          /* ------------------------------------------------------------------
           *    Receive succeeded
           *   -------------------
           */

          /* call external, higher level "receive complete" processing routine */
          MRFI_RxCompleteISR();
        }
      }
    }
  }

  /* ------------------------------------------------------------------
   *    End of function
   *   -------------------
   */
  
  /* clear "receive active" flag and exit */
  mrfiRxActive = 0;
}


/**************************************************************************************************
 * @fn          MRFI_Sleep
 *
 * @brief       Request radio go to sleep.
 *
 * @param       none
 *
 * @return      zero : if successfully went to sleep
 *              non-zero : if sleep was not entered
 **************************************************************************************************
 */
uint8_t MRFI_Sleep(void)
{
  /* if radio is already asleep just indicate radio went to sleep successfully */
  if (mrfiRadioIsSleeping)
  {
    /* return value of zero indicates sleep state was entered */
    return( 0 );
  }

  /* determine if sleep is possible and return the corresponding code */
  {
    bspIState_t s;

    /* critical section necessary for watertight testing and setting of state variables */
    BSP_ENTER_CRITICAL_SECTION(s);
    if (!mrfiTxActive && !mrfiRxActive)
    {
      mrfiRadioIsSleeping = 1;
      MRFI_DISABLE_SYNC_PIN_INT();
      mrfiSpiCmdStrobe(MRFI_CC2500_SPI_STROBE_SIDLE);
      mrfiSpiCmdStrobe(MRFI_CC2500_SPI_STROBE_SPWD);
      BSP_EXIT_CRITICAL_SECTION(s);
      /* return value of zero indicates sleep state was entered */
      return( 0 );
    }
    else
    {
      BSP_EXIT_CRITICAL_SECTION(s);
      /* return value of non-zero indicates sleep state was *not* entered */
      return( 1 );
    }
  }
}


/**************************************************************************************************
 * @fn          MRFI_WakeUp
 *
 * @brief       Wake up radio from sleep state.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void MRFI_WakeUp(void)
{
  /* if radio is already awake, just ignore wakeup request */
  if (!mrfiRadioIsSleeping)
  {
    return;
  }

  /* drive CSn low to initiate wakeup */
  MRFI_SPI_DRIVE_CSN_LOW();
  
  /* wait for MISO to go high indicating the oscillator is stable */
  while (MRFI_SPI_SO_IS_HIGH());
  
  /* wakeup is complete, drive CSn high and continue */
  MRFI_SPI_DRIVE_CSN_HIGH();

/*
 *  The test registers must be restored after sleep for the CC1100 and CC2500 radios.
 *  This is not required for the CC1101 radio.
 */
#ifndef MRFI_CC1101
  mrfiSpiWriteReg( MRFI_CC2500_SPI_REG_TEST2, SMARTRF_SETTING_TEST2 );
  mrfiSpiWriteReg( MRFI_CC2500_SPI_REG_TEST1, SMARTRF_SETTING_TEST1 );
  mrfiSpiWriteReg( MRFI_CC2500_SPI_REG_TEST0, SMARTRF_SETTING_TEST0 );
#endif

  /* clear any residual SYNC pin interrupt and then re-enable SYNC pin interrupts */
  MRFI_CLEAR_SYNC_PIN_INT_FLAG();
  MRFI_ENABLE_SYNC_PIN_INT();

  /* clear sleep flag and enter receive mode */
  mrfiRadioIsSleeping = 0;
  mrfiSpiCmdStrobe(MRFI_CC2500_SPI_STROBE_SRX);
}


/**************************************************************************************************
 * @fn          MRFI_SetRxAddrFilter
 *
 * @brief       Set the address used for filtering received packets.
 *
 * @param       pAddr - pointer to address to use for filtering
 *
 * @return      none
 **************************************************************************************************
 */
void MRFI_SetRxAddrFilter(uint8_t * pAddr)
{
  /*
   *  Set the hardware address register.  The hardware address filtering only recognizes
   *  a single byte but this does provide at least some automatic hardware filtering.
   */
  mrfiSpiWriteReg( MRFI_CC2500_SPI_REG_ADDR, pAddr[0] );

  /* save a copy of the filter address */
  memcpy(&mrfiRxFilterAddr[0], pAddr, MRFI_ADDR_SIZE);
}


/**************************************************************************************************
 * @fn          MRFI_EnableRxAddrFilter
 *
 * @brief       Enable received packet filtering.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void MRFI_EnableRxAddrFilter(void)
{
  /* set flag to indicate filtering is enabled */
  mrfiRxFilterEnabled = 1;

  /* enable hardware filtering on the radio */
  mrfiSpiWriteReg( MRFI_CC2500_SPI_REG_PKTCTRL1, MRFI_SETTING_PKTCTRL1_ADDR_FILTER_ON );
}


/**************************************************************************************************
 * @fn          MRFI_DisableRxAddrFilter
 *
 * @brief       Disable received packet filtering.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void MRFI_DisableRxAddrFilter(void)
{
  /* clear flag that indicates filtering is enabled */
  mrfiRxFilterEnabled = 0;

  /* disable hardware filtering on the radio */
  mrfiSpiWriteReg( MRFI_CC2500_SPI_REG_PKTCTRL1, MRFI_SETTING_PKTCTRL1_ADDR_FILTER_OFF );
}


/**************************************************************************************************
 * @fn          MRFI_GpioIsr
 *
 * @brief       Interrupt Service Routine for handling GPIO interrupts.  The sync pin interrupt
 *              comes in through GPIO.  This function is designed to be compatible with "ganged"
 *              interrupts.  If the GPIO interrupt services more than just a single pin (very
 *              common), this function just needs to be called from the higher level interrupt
 *              service routine.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void MRFI_GpioIsr(void)
{
  /* see if sync pin interrupt is enabled and has fired */
  if (MRFI_SYNC_PIN_INT_IS_ENABLED() && MRFI_SYNC_PIN_INT_FLAG_IS_SET())
  {
    /*  clear the sync pin interrupt, run sync pin ISR */

    /*
     *  NOTE!  The following macro clears the interrupt flag but it also *must*
     *  reset the interrupt capture.  In other words, if a second interrupt
     *  occurs after the flag is cleared it must be processed, i.e. this interrupt
     *  exits then immediately starts again.  Most microcontrollers handle this
     *  naturally but it must be verified for every target.
     */
    MRFI_CLEAR_SYNC_PIN_INT_FLAG();
    MRFI_SyncPinRxIsr();
  }
}


/* ================================================================================================
 *                                        C Code Includes
 * ================================================================================================
 */
#include "bsp_external/mrfi_board.c"


/**************************************************************************************************
 *                                  Compile Time Integrity Checks
 **************************************************************************************************
 */

/* calculate maximum value for PKTLEN and verify it directly */
#define MRFI_RADIO_TX_FIFO_SIZE     64  /* from datasheet */
#define MRFI_RADIO_MAX_PKTLEN       (MRFI_RADIO_TX_FIFO_SIZE - MRFI_LENGTH_FIELD_SIZE - MRFI_RX_METRICS_SIZE)
#if (MRFI_RADIO_MAX_PKTLEN != 61)
#error "ERROR:  The maximum value for PKTLEN is not correct."
#endif

/* verify setting for PKTLEN does not exceed maximum */
#if (MRFI_SETTING_PKTLEN > MRFI_RADIO_MAX_PKTLEN)
#error "ERROR:  Maximum possible value for PKTLEN exceeded.  Decrease value of maximum payload."
#endif

/* verify largest possible packet fits within FIFO buffer */
#if ((MRFI_MAX_FRAME_SIZE + MRFI_RX_METRICS_SIZE) > MRFI_RADIO_TX_FIFO_SIZE)
#error "ERROR:  Maximum possible packet length exceeds FIFO buffer.  Decrease value of maximum payload."
#endif

/* verify that the SmartRF file supplied is compatible */
#if ((!defined SMARTRF_RADIO_CC2500) && \
     (!defined SMARTRF_RADIO_CC1100) && \
     (!defined SMARTRF_RADIO_CC1101))
#error "ERROR:  The SmartRF export file is not compatible."
#endif

/**************************************************************************************************
*/
