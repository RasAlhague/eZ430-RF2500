#include <stdio.h>
#include "bsp.h"
#include "mrfi.h"
#include "bsp_leds.h"
#include "gradient.h"
#include "printing.h"
#include "bsp_buttons.h"
#include "sensing.h"

uint8_t wakeup_cycles_before_send = SEND_PERIOD;

int main(void)
{
  WDTCTL = WDTPW + WDTHOLD;
  BSP_Init();
  MRFI_Init();
  gradient_Init(IS_SINK_NODE);
  __bis_SR_register(GIE+LPM0_bits);
}

#pragma vector=TIMERA0_VECTOR
__interrupt void Timer_A0 (void)
{
  print_interrupt("\r\nTIMERA0",9);
  // change TIMERA0 period randomly
  uint16_t fixed_part, random_part, random;
  fixed_part     = 1500*WAKEUP_PERIOD_S_FIXED;
  random_part    = WAKEUP_PERIOD_S_RANDOM;
  if (WAKEUP_PERIOD_S_RANDOM!=0) {
    random_part |= MRFI_RandomByte();
    random_part |= (MRFI_RandomByte()<<8);
    random_part  = random_part%(1500*WAKEUP_PERIOD_S_RANDOM);
  }
  random         = fixed_part + random_part;
  TACTL = MC_0;
  TAR = 0;
  TACCR0 = random;
  TACTL = TASSEL_1 + MC_1 + ID_3;
  //add data message to buffer
  wakeup_cycles_before_send--;
  if (wakeup_cycles_before_send==0) {
    gradient_generate_data();
    wakeup_cycles_before_send=SEND_PERIOD;
  }
  //send oldest buffered message
  gradient_send_data();
}

#pragma vector=TIMERB0_VECTOR
__interrupt void Timer_B0 (void)
{
  print_interrupt("\r\nTIMERB0",9);
  gradient_wakeup_timer();
}

#pragma vector=TIMERB1_VECTOR
__interrupt void Timer_B1 (void)
{
  print_interrupt("\r\nTIMERB1",9);
  gradient_wakeup_timer();
}

void MRFI_RxCompleteISR()
{
  print_interrupt("\r\nRX",4);
  BSP_TOGGLE_LED1();
  gradient_wakeup_rx();
  BSP_TOGGLE_LED1();
}

#pragma vector=PORT1_VECTOR
__interrupt void Port_1 (void)
{
  P1IFG &= ~0x04;                           // clear interrupt flag
  print_interrupt("\r\nPORT1",7);
}

#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void)
{
  print_interrupt("\r\nADC10",7);
  __bic_SR_register_on_exit(CPUOFF);        // Clear CPUOFF bit from SR
}

/*#pragma vector=USCIAB0RX_VECTOR
__interrupt void serialRx (void)
{
  P1OUT ^= 0x03;
  IFG2 &= ~0x01;
}

#pragma vector=USCIAB0TX_VECTOR
__interrupt void serialTx (void)
{
  P1OUT ^= 0x03;
  IFG2 &= ~0x02;
}*/