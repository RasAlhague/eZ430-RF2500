#include "io430.h"
#include "in430.h"
int main( void )
{
  WDTCTL = WDTPW + WDTHOLD;
  int i;
  int y;
  P1DIR |= 0x03;
  while (1) {
    P1OUT ^= 0x03;
    for (i=0;i<100;i++) {
      for (y=0;y<10000;y++) {
        __no_operation();
      }
    }
  }
}