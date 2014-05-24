#include "io430.h"
int main( void )
{
  WDTCTL = WDTPW + WDTHOLD;
  P1DIR |= 0x03;
  P1OUT |= 0x03;
  while(1);
}