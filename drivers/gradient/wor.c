#include "mrfi.h"
#include "bsp.h"
#include "gradient.h"
#include "printing.h"
#include "radios/family1/mrfi_spi.h"
#include "wor.h"

void wor_start(uint8_t is_sink)
{
  if(is_sink==0) {
    //choose EVENT0 (=(uint16_t)(WOREVT1<<8)+WOREVT10)   == 3600=0x0E10 (103.5ms)
    mrfiSpiWriteReg(WOREVT1, 0x0E);
    mrfiSpiWriteReg(WOREVT0, 0x10);
    //choose EVENT1[WORCTRL.6-4]                         == 7=0b111
    uint8_t reg  = mrfiSpiReadReg(WORCTRL);
    reg |=  0x70;
    mrfiSpiWriteReg(WORCTRL, reg);
    //enable frequence calibration RC_CAL[WORCTRL.3]     == 1
    reg  = mrfiSpiReadReg(WORCTRL);
    reg |=  0x80;
    mrfiSpiWriteReg(WORCTRL, reg);
    //choose WOR_RES[WORCTRL.1-0]                        == 00
    reg  = mrfiSpiReadReg(WORCTRL);
    reg &= ~0x03;
    mrfiSpiWriteReg(WORCTRL, reg);
    //choose when to calibrate FS_AUTOCAL[MCSM0.5-4]     == 01
    reg  = mrfiSpiReadReg(MCSM0);
    reg &= ~0x20;
    reg |=  0x10;
    mrfiSpiWriteReg(MCSM0, reg);
    //initial filter by rssi RX_TIME_RSSI[MCSM2.4]       == 0
    reg  = mrfiSpiReadReg(MCSM2);
    reg &= ~0x10;
    mrfiSpiWriteReg(MCSM2, reg);
    //stay in Rx after preamble RX_TIME_QUAL[MCSM2.3]    == 0
    reg  = mrfiSpiReadReg(MCSM2);
    reg &= ~0x08;
    mrfiSpiWriteReg(MCSM2, reg);
    //choose RX_TIME[MCSM2.2-0]                          == 2=0b010
    reg  = mrfiSpiReadReg(MCSM2);
    reg &= ~0x05;
    reg |=  0x02;
    mrfiSpiWriteReg(MCSM2, reg);
    //enable RC oscilator RC_PD[WORCTRL.7]               == 0
    reg  = mrfiSpiReadReg(WORCTRL);
    reg &= ~0x80;
    mrfiSpiWriteReg(WORCTRL, reg);
    print_cc2500_wor_registers();
    //from IDLE state, enter WOR*/
    MRFI_RxIdle();
    MRFI_RxWor();
  } else {
    MRFI_RxIdle();
    MRFI_RxOn();
  }
  print_cc2500_wor_status("\r\nWOR started",13);
}
void wor_stop(uint8_t is_sink)
{
  if(is_sink==0) {
    //turn radio to IDLE, quitting WOR
    MRFI_RxIdle();
    //disable RC oscilator RC_PD[WORCTRL.7]             == 1
    uint8_t reg  = mrfiSpiReadReg(WORCTRL);
    reg |= 0x80;
    mrfiSpiWriteReg(WORCTRL, reg);
    //choose RX_TIME[MCSM2.2-0]                          == 2=0b111
    reg  = mrfiSpiReadReg(MCSM2);
    reg |=  0x07;
    mrfiSpiWriteReg(MCSM2, reg);
    //choose EVENT0 (=(uint16_t)(WOREVT1<<8)+WOREVT10)   == 3600=0x876B (default)
    mrfiSpiWriteReg(WOREVT1, 0x87);
    mrfiSpiWriteReg(WOREVT0, 0x6B);
    //choose EVENT1[WORCTRL.6-4]                         == 7=0b111
    reg  = mrfiSpiReadReg(WORCTRL);
    reg |=  0x70;
    mrfiSpiWriteReg(WORCTRL, reg);
    //enable frequence calibration RC_CAL[WORCTRL.3]     == 1
    reg  = mrfiSpiReadReg(WORCTRL);
    reg |=  0x80;
    mrfiSpiWriteReg(WORCTRL, reg);
    //choose WOR_RES[WORCTRL.1-0]                        == 00
    reg  = mrfiSpiReadReg(WORCTRL);
    reg &= ~0x03;
    mrfiSpiWriteReg(WORCTRL, reg);
    //choose when to calibrate FS_AUTOCAL[MCSM0.5-4]     == 01
    reg  = mrfiSpiReadReg(MCSM0);
    reg &= ~0x20;
    reg |=  0x10;
    mrfiSpiWriteReg(MCSM0, reg);
    //initial filter by rssi RX_TIME_RSSI[MCSM2.4]       == 0
    reg  = mrfiSpiReadReg(MCSM2);
    reg &= ~0x10;
    mrfiSpiWriteReg(MCSM2, reg);
    //stay in Rx after preamble RX_TIME_QUAL[MCSM2.3]    == 0
    reg  = mrfiSpiReadReg(MCSM2);
    reg &= ~0x08;
    mrfiSpiWriteReg(MCSM2, reg);
  }
  print_cc2500_wor_registers();
  print_cc2500_wor_status("\r\nWOR stopped",13);
}