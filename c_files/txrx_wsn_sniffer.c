#include <stdio.h>
#include "bsp.h"
#include "mrfi.h"
#include "bsp_leds.h"
#include "bsp_buttons.h"
#include "gradient.h"
#include "printing.h"

mrfiPacket_t inBuffer;

void print_time();

int main(void)
{
  WDTCTL = WDTPW + WDTHOLD;            // stop watchdog timer
  BSP_Init();
  MRFI_Init();
  MCU_Init(30000);                     //30000 is dummy large number
  MRFI_WakeUp();                       //start radio in idle state
  MRFI_RxOn();                         //put radio in Rx
  __bis_SR_register(LPM3_bits + GIE);
}

void MRFI_RxCompleteISR()
{
  BSP_TOGGLE_LED1();
  MRFI_Receive(&inBuffer);
  uint8_t pkt_type = GET_FROM_FRAME(MRFI_P_PAYLOAD(&inBuffer), F_TYPE);
  switch (pkt_type) {
  case (F_TYPE_UF):
#ifdef PRINT_UF
    print_time();
    print_UF(&inBuffer);
#endif
    break;
  case (F_TYPE_CW):
#ifdef PRINT_CW
    print_time();
    print_CW(&inBuffer);
#endif
    break;
  case (F_TYPE_ACK):
#ifdef PRINT_ACK
    print_time();
    print_ACK(&inBuffer);
#endif
    break;
  case (F_TYPE_DATA):
#ifdef PRINT_DATA
    print_time();
    print_DATA(&inBuffer);
#endif
    break;
  case (F_TYPE_FIN):
#ifdef PRINT_FIN
    print_time();
    print_FIN(&inBuffer);
#endif
    break;
  default:
    print_time();
    print_debug("\r\nError: Unrecognized packet format",35);
  }
  BSP_TOGGLE_LED1();
}

#pragma vector=TIMERA0_VECTOR
__interrupt void Timer_A0 (void)
{
  BSP_TOGGLE_LED2();
}

#pragma vector=PORT1_VECTOR
__interrupt void Port_1 (void)
{
  P1IFG &= ~0x04;                           // P1.3 IFG cleared
}

void print_time()
{
  uint16_t time = TAR/12; //as it's a 12kHz clock, these are ms
  char output[] = {"\r\ntime=XX.XXX s"};
  output[7]  = '0'+((time/10000)%10);
  output[8]  = '0'+((time/1000)%10);
  output[10] = '0'+((time/100)%10);
  output[11] = '0'+((time/10)%10);
  output[12] = '0'+(time%10);
  TXString(output, sizeof output);
}
