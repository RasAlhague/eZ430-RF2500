/**************************************************************************************************
  Filename:       nwk.h
  Revised:        $Date: 2007-10-14 16:14:33 -0700 (Sun, 14 Oct 2007) $
  Revision:       $Revision: 15682 $
  Author:         $Author: lfriedman $

  Description:    This header file supports the SimpliciTI network layer.

  Copyright 2004-2007 Texas Instruments Incorporated. All rights reserved.

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

#ifndef NWK_H
#define NWK_H


// well known ports
#define SMPL_PORT_PING          0x01
#define SMPL_PORT_LINK          0x02
#define SMPL_PORT_JOIN          0x03
#define SMPL_PORT_SECURITY      0x04
#define SMPL_PORT_FREQ          0x05
#define SMPL_PORT_MGMT          0x06
#define SMPL_PORT_NWK_BCAST     0x1F

#define SMPL_PORT_USER_MAX      0x3D
#define SMPL_PORT_TX_ONLY       0x3E
#define SMPL_PORT_USER_BCAST    0x3F

// Unconnected User Datagram Link ID
#define SMPL_LINKID_USER_UUD  ((linkID_t)~0)

#define PORT_BASE_NUMBER       0x20

// to check connection info sanity
#define CHK_RX   0
#define CHK_TX   1

// when allocating local Rx port it depends on whether the allocation
// is being done as a result of a link or a link reply
#define LINK_SEND   1
#define LINK_REPLY  2

#define IS_BCAST(a)          ((0xFF == (a)) || (0x00 == (a)))
#define MAP_LID2IDX(lid)     ((lid==SMPL_LINKID_USER_UUD) ? 0 : lid)

typedef struct
{
  union
  {
    volatile uint8_t    isValid;
    volatile uint8_t    linkNum;
  };
           uint8_t    hops2target;
           uint8_t    peerAddr[NET_ADDR_SIZE];
           uint8_t    sigInfo[MRFI_RX_METRICS_SIZE];
           uint8_t    portRx;
           uint8_t    portTx;
           linkID_t   thisLinkID;
} connInfo_t;


// prototypes
smplStatus_t  nwk_nwkInit(uint8_t (*)(linkID_t));
connInfo_t   *nwk_getNextConnection(void);
void          nwk_freeConnection(connInfo_t *);
uint8_t       nwk_getNextClientPort(void);
connInfo_t   *nwk_getConnInfo(linkID_t port);
connInfo_t   *nwk_isLinkDuplicate(uint8_t *, uint8_t);
connInfo_t   *nwk_findAddressMatch(mrfiPacket_t *);
smplStatus_t  nwk_checkConnInfo(connInfo_t *, uint8_t);
uint8_t       nwk_isConnectionValid(mrfiPacket_t *, linkID_t *);
uint8_t       nwk_allocateLocalRxPort(uint8_t, connInfo_t *);
linkID_t      nwk_rxPort2linkID(uint8_t);
uint8_t       nwk_linkID2RxPort(linkID_t);

#endif
