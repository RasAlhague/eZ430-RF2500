#ifndef NWK_QMGMT_H
#define NWK_QMGMT_H

#define  INQ   1
#define  OUTQ  2

#define   FI_AVAILABLE         0   /* entry available for use */
#define   FI_INUSE             1   /* entry in use */

typedef struct
{
  volatile uint8_t      fi_usage;
  volatile uint8_t      retries;
           mrfiPacket_t mrfiPkt;
} frameInfo_t;

frameInfo_t* QfindFreeSlot(uint8_t);
frameInfo_t* QgetInUseSlot(uint8_t);
void         QfreeSlot(frameInfo_t *);
void         Qprint();
#endif