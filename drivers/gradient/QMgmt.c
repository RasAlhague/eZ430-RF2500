#include "bsp.h"
#include "mrfi.h"
#include "QMgmt.h"
#include "gradient.h"
#include "printing.h"

static frameInfo_t sInFrameQ[SIZE_Q];
static frameInfo_t sOutFrameQ[SIZE_Q];

/* returns a pointer to a free slot, or *0 if all in use
 * when slot is free, set it as in use
 * start searching from the start of the queue (to achieve FIFO queueing) */
frameInfo_t *QfindFreeSlot(uint8_t which)
{
  frameInfo_t *pFI, *newFI = 0;
  uint8_t i;
  // iterate through the queue
  if (INQ == which) { pFI  = sInFrameQ; } else { pFI  = sOutFrameQ; }
  for (i=0; i<SIZE_Q; ++i, ++pFI) {
    if (pFI->fi_usage == FI_AVAILABLE) {
      pFI->fi_usage = FI_INUSE;
      pFI->retries = 0;
      return pFI;
    }
  }
  return newFI;
}

/* returns a pointer to a slot in use, or *0 if all free
 * start searching from the start of the queue (to achieve FIFO queueing) */
frameInfo_t *QgetInUseSlot(uint8_t which)
{
  frameInfo_t *pFI, *newFI = 0;
  uint8_t i, retries_youngest;
  //remove old messages
  if (INQ == which) { pFI  = sInFrameQ; } else { pFI  = sOutFrameQ; }
  for (i=0; i<SIZE_Q; ++i, ++pFI) {
    if ((pFI->fi_usage == FI_INUSE) && (pFI->retries>MAX_RETRIES-1)) {
      QfreeSlot(pFI);
    }
  }
  //detect youngest message
  if (INQ == which) { pFI  = sInFrameQ; } else { pFI  = sOutFrameQ; }
  for (i=0; i<SIZE_Q; ++i, ++pFI) {
    retries_youngest = MAX_RETRIES+1;
    if ((pFI->fi_usage == FI_INUSE) && (pFI->retries<retries_youngest)) {
      retries_youngest = pFI->retries;
      newFI = pFI;
    }
  }
  //increase the number of retries of the select message
  if (newFI) {
    newFI->retries++;
  }
  return newFI;
}

/* frees a given slot */
void QfreeSlot(frameInfo_t *pslot)
{
  pslot->fi_usage = FI_AVAILABLE;
  pslot->retries = 0;
}

/* prints the state of the queues' elements,
 * including the number of retries for OUTQ when slot in use */
void Qprint()
{
#ifdef PRINT_Q
  frameInfo_t *pFI;
  uint8_t  i;
  char output_in_use[] = {" (X)IN_USE"};
  /* print INQ */
  pFI  = sInFrameQ;
  print_debug("\r\nINQ: ",7);
  for (i=0; i<SIZE_Q; ++i, ++pFI)
  {
    if (pFI->fi_usage == FI_AVAILABLE) {
      print_debug(" AVAILABLE",10);
    } else {
      print_debug(" IN_USE",7);
    }
  }
  /* print OUTQ */
  pFI  = sOutFrameQ;
  print_debug("\r\nOUTQ:",7);
  for (i=0; i<SIZE_Q; ++i, ++pFI)
  {
    if (pFI->fi_usage == FI_AVAILABLE) {
      print_debug(" AVAILABLE",10);
    } else {
      output_in_use[2] = '0'+pFI->retries;
      TXString(output_in_use, sizeof output_in_use);
    }
  }
#endif
}