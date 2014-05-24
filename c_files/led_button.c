#include "io430.h"
#include "in430.h"
int main( void )
{
  WDTCTL = WDTPW + WDTHOLD;
  P1DIR |=  0x03;
  P1DIR &= ~0x04;
  P1REN |= 0x04;
  P1IE |= 0x04;
  __bis_SR_register(GIE);
  while(1);
}
#pragma vector=PORT1_VECTOR
__interrupt void Port_1 (void)
{
  P1IFG &= ~0x04;
  P1OUT ^= 0x03;
}