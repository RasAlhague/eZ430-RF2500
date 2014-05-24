#include "mrfi.h"
#include "bsp.h"
#include "gradient.h"
#include "radios/family1/mrfi_spi.h"
#include "QMgmt.h"
#include "printing.h"

//=============================================
void print_debug(char* msg, uint8_t length)
{
#ifdef PRINT_DEBUG
  TXString(msg, length);
#endif
}

//=============================================
void print_sink(char* msg, uint8_t length)
{
#ifdef PRINT_SINK
  TXString(msg, length);
#endif
}

//=============================================
void print_fail(char* msg, uint8_t length)
{
#ifdef PRINT_FAIL
  TXString(msg, length);
#endif
}

//=============================================
void print_success(char* msg, uint8_t length)
{
#ifdef PRINT_SUCCESS
  TXString(msg, length);
#endif
}

//=============================================
void print_interrupt(char* msg, uint8_t length)
{
#ifdef PRINT_INTERRUPTS
  TXString(msg, length);
#endif
}

//=============================================
void print_cc2500_wor_status(char* msg, uint8_t length)
{
#ifdef PRINT_CC2500_WOR_STATUS
  TXString(msg, length);
#endif
}

//=============================================
void print_height(uint8_t addr, uint8_t height)
{
#ifdef PRINT_HEIGHT
  char output[] = {"\r\n    node XXX, height XXX"};
  output[11]  = '0'+((addr/100)%10);
  output[12] = '0'+((addr/10)%10);
  output[13] = '0'+(addr%10);
  output[23] = '0'+((height/100)%10);
  output[24] = '0'+((height/10)%10);
  output[25] = '0'+(height%10);
  TXString(output, sizeof output);
#endif
}

//=============================================
void print_neighbor_table(neighbor_t* pneighbors)
{
#ifdef PRINT_NEIGHBOR_TABLE
  uint8_t i;
  int8_t rssi;
  for (i=0; i<MAX_NEIGHBORS; i++) {
    if (pneighbors[i].addr!=0) {
      char output[] = {"\r\n  neighbor XXX, height XXX, rssi  XXX"};
      output[13] = '0'+((pneighbors[i].addr/100)%10);
      output[14] = '0'+((pneighbors[i].addr/10)%10);
      output[15] = '0'+(pneighbors[i].addr%10);
      output[25] = '0'+((pneighbors[i].height/100)%10);
      output[26] = '0'+((pneighbors[i].height/10)%10);
      output[27] = '0'+(pneighbors[i].height%10);
      rssi = pneighbors[i].rssi;
      if (rssi<0) {
        rssi=-rssi;
        output[35] = '-';
      }
      output[36] = '0'+((rssi/100)%10);
      output[37] = '0'+((rssi/10)%10);
      output[38] = '0'+(rssi%10);
      TXString(output, sizeof output);
    }
  }
#endif
}

//=============================================
void print_gradient_state(uint8_t state)
{
#ifdef PRINT_GRADIENT_STATE
  switch(state) {
  case GRADIENT_IDLE:
    print_debug("\r\nstate=IDLE",12);
    break;
  case GRADIENT_TXUF:
    print_debug("\r\nstate=TXUF",12);
    break;
  case GRADIENT_TXCW:
    print_debug("\r\nstate=TXCW",12);
    break;
  case GRADIENT_RXACK:
    print_debug("\r\nstate=RXACK",13);
    break;
  case GRADIENT_TXDATA:
    print_debug("\r\nstate=TXDATA",14);
    break;
  case GRADIENT_RXFIN:
    print_debug("\r\nstate=RXFIN",13);
    break;
  case GRADIENT_RXUF:
    print_debug("\r\nstate=RXUF",12);
    break;
  case GRADIENT_RXCW:
    print_debug("\r\nstate=RXCW",12);
    break;
  case GRADIENT_BACKOFFACK:
    print_debug("\r\nstate=BACKOFFACK",18);
    break;
  case GRADIENT_TXACK:
    print_debug("\r\nstate=TXACK",13);
    break;
  case GRADIENT_SENTACK:
    print_debug("\r\nstate=SENTACK",15);
    break;
  case GRADIENT_RXDATA:
    print_debug("\r\nstate=RXDATA",14);
    break;
  case GRADIENT_TXFIN:
    print_debug("\r\nstate=TXFIN",13);
    break;
  }
#endif
}

/*=============================================
 * Ojo: using this function when WOR is enabled will cancel WOR because
 * bringing CS down triggers IDLE mode
**/
void print_cc2500_status()
{
#ifdef PRINT_CC2500_STATUS
  uint8_t status = mrfiSpiReadReg(0x35);
  char output[] = {"\r\nCC2500 status = XX"};
  output[18]  = '0'+((status/10)%10);
  output[19]  = '0'+((status)%10);
  TXString(output, sizeof output);
#endif
}

//=============================================
void print_cc2500_registers()
{
#ifdef PRINT_CC2500_REGISTERS
  uint8_t reg=0x00, value, reg_msB, reg_lsB, value_msB, value_lsB;
  //-----------------registers
  char output_1[] = {"\r\nCC2500 registers:"};
  TXString(output_1, sizeof output_1);
  char output_2[] = {"\r\n0xXX = XXXX XXXX"};
  for (reg=0; reg<=0x3d; reg++) {
    value = mrfiSpiReadReg(reg);
    reg_msB = ((reg & 0xF0)>>4);
    reg_lsB = ( reg & 0x0F);
    if (reg_msB<10) {
      output_2[4]='0'+reg_msB;
    } else {
      output_2[4]='a'+reg_msB-10;
    }
    if (reg_lsB<10) {
      output_2[5]='0'+reg_lsB;
    } else {
      output_2[5]='a'+reg_lsB-10;
    }
    output_2[9]  = '0'+((value & (1<<7))>>7);
    output_2[10] = '0'+((value & (1<<6))>>6);
    output_2[11] = '0'+((value & (1<<5))>>5);
    output_2[12] = '0'+((value & (1<<4))>>4);
    output_2[14] = '0'+((value & (1<<3))>>3);
    output_2[15] = '0'+((value & (1<<2))>>2);
    output_2[16] = '0'+((value & (1<<1))>>1);
    output_2[17] = '0'+((value & (1<<0))>>0);
    TXString(output_2, sizeof output_2);
  }
  //-----------------PATABLE
  char output_3[] = {"\r\nCC2500 PATABLE(0): "};
  TXString(output_3, sizeof output_3);
  char output_4[] = {"XX"};   
  //DUMMY_BYTE=0xDB, READ_BIT=0x80, BURST_BIT=0x40
  value = spiRegAccess(PATABLE | 0x40 | 0x80, 0xDB);
  value_msB = ((value & 0xF0)>>4);
  value_lsB = ( value & 0x0F);
  if (value_msB<10) {
    output_4[0]='0'+value_msB;
  } else {
    output_4[0]='a'+value_msB-10;
  }
  if (value_lsB<10) {
    output_4[1]='0'+value_lsB;
  } else {
    output_4[1]='a'+value_lsB-10;
  }
  TXString(output_4, sizeof output_4);
#endif
}

//=============================================
void print_cc2500_wor_registers()
{
#ifdef PRINT_CC2500_WOR_REGISTERS
  uint8_t  value;
  uint16_t bigvalue;
  //---------------------------------
  char output_1[] = {"\r\n[WOREVT1:WOREVT0] EVENT0         XXXXX"};
  bigvalue  = (mrfiSpiReadReg(WOREVT1)<<8);
  bigvalue +=  mrfiSpiReadReg(WOREVT0);
  output_1[35]    = '0'+((bigvalue/10000)%10);
  output_1[36]    = '0'+((bigvalue/1000)%10);
  output_1[37]    = '0'+((bigvalue/100)%10);
  output_1[38]    = '0'+((bigvalue/10)%10);
  output_1[39]    = '0'+((bigvalue/1)%10);
  TXString(output_1, sizeof output_1);
  //---------------------------------
  char output_2[] = {"\r\n[WORCTRL.7]       RC_PD          X"};
  value = mrfiSpiReadReg(WORCTRL);
  output_2[35]    = '0'+((value & (1<<7))>>7);
  TXString(output_2, sizeof output_2);
  //---------------------------------
  char output_3[] = {"\r\n[WORCTRL.6-4]     EVENT1         XXX"};
  value = mrfiSpiReadReg(WORCTRL);
  output_3[35]    = '0'+((value & (1<<6))>>6);
  output_3[36]    = '0'+((value & (1<<5))>>5);
  output_3[37]    = '0'+((value & (1<<4))>>4);
  TXString(output_3, sizeof output_3);
  //---------------------------------
  char output_4[] = {"\r\n[WORCTRL.3]       RC_CAL         X"};
  value = mrfiSpiReadReg(WORCTRL);
  output_4[35]    = '0'+((value & (1<<3))>>3);
  TXString(output_4, sizeof output_4);
  //---------------------------------
  char output_5[] = {"\r\n[WORCTRL.1-0]     WOR_RES        XX"};
  value = mrfiSpiReadReg(WORCTRL);
  output_5[35]    = '0'+((value & (1<<1))>>1);
  output_5[36]    = '0'+((value & (1<<0))>>0);
  TXString(output_5, sizeof output_5);
  //---------------------------------
  char output_6[] = {"\r\n[MCSM0.5-4]       FS_AUTOCAL     XX"};
  value = mrfiSpiReadReg(MCSM0);
  output_6[35]    = '0'+((value & (1<<5))>>5);
  output_6[36]    = '0'+((value & (1<<4))>>4);
  TXString(output_6, sizeof output_6);
  //---------------------------------
  char output_7[] = {"\r\n[MCSM2.4]         RX_TIME_RSSI   X"};
  value = mrfiSpiReadReg(MCSM2);
  output_7[35]    = '0'+((value & (1<<4))>>4);
  TXString(output_7, sizeof output_7);
  //---------------------------------
  char output_8[] = {"\r\n[MCSM2.3]         RX_TIME_QUAL   X"};
  value = mrfiSpiReadReg(MCSM2);
  output_8[35]    = '0'+((value & (1<<3))>>3);
  TXString(output_8, sizeof output_8);
  //---------------------------------
  char output_9[] = {"\r\n[MCSM2.2-0]       RX_TIME        XXX"};
  value = mrfiSpiReadReg(MCSM2);
  output_9[35]    = '0'+((value & (1<<2))>>2);
  output_9[36]    = '0'+((value & (1<<1))>>1);
  output_9[37]    = '0'+((value & (1<<0))>>0);
  TXString(output_9, sizeof output_9);
#endif
}

//=============================================
void print_UF(mrfiPacket_t * ppacketReceived)
{
#ifdef PRINT_UF
  uint8_t source  = ppacketReceived->frame[F_SRC];
  uint8_t counter = ppacketReceived->frame[F_UF_COUNTER];
  int8_t  rssi    = ppacketReceived->rxMetrics[0];
  char output[] = {"\r\nUF from XXX, counter=XXX (rssi= XXX)"};
  output[10] = '0'+((source/100)%10);
  output[11] = '0'+((source/10)%10);
  output[12] = '0'+(source%10);
  output[23] = '0'+((counter/100)%10);
  output[24] = '0'+((counter/10)%10);
  output[25] = '0'+(counter%10);
  if (rssi<0) {
    rssi=-rssi;
    output[33] = '-';
  }
  output[34] = '0'+((rssi/100)%10);
  output[35] = '0'+((rssi/10)%10);
  output[36] = '0'+(rssi%10);
  TXString(output, sizeof output);
#endif
}

void print_CW(mrfiPacket_t * ppacketReceived)
{
#ifdef PRINT_CW
  uint8_t source     = ppacketReceived->frame[F_SRC];
  uint8_t cwduration = ppacketReceived->frame[F_CW_CWDURATION];
  int8_t  rssi       = ppacketReceived->rxMetrics[0];
  char output[] = {"\r\nCW from XXX, cwduration=XXX (rssi= XXX)"};
  output[10] = '0'+((source/100)%10);
  output[11] = '0'+((source/10)%10);
  output[12] = '0'+(source%10);
  output[26] = '0'+((cwduration/100)%10);
  output[27] = '0'+((cwduration/10)%10);
  output[28] = '0'+(cwduration%10);
  if (rssi<0) {
    rssi=-rssi;
    output[35] = '-';
  }
  output[37] = '0'+((rssi/100)%10);
  output[38] = '0'+((rssi/10)%10);
  output[39] = '0'+(rssi%10);
  TXString(output, sizeof output);
#endif
}

void print_ACK(mrfiPacket_t * ppacketReceived)
{
#ifdef PRINT_ACK
  uint8_t source = ppacketReceived->frame[F_SRC];
  int8_t  rssi   = ppacketReceived->rxMetrics[0];
  char output[] = {"\r\nACK from XXX (rssi= XXX)"};
  output[22] = '0'+((source/100)%10);
  output[23] = '0'+((source/10)%10);
  output[24] = '0'+(source%10);
  if (rssi<0) {
    rssi=-rssi;
    output[11] = '-';
  }
  output[12] = '0'+((rssi/100)%10);
  output[13] = '0'+((rssi/10)%10);
  output[14] = '0'+(rssi%10);
  TXString(output, sizeof output);
#endif
}

void print_DATA(mrfiPacket_t * ppacketReceived, uint8_t myAddr)
{
#ifdef PRINT_DATA
  uint8_t current_output_index = 0;
  char output[200];
  //myAddr
  output[current_output_index]  = '0'+((myAddr/100)%10);
  output[++current_output_index]  = '0'+((myAddr/10)%10);
  output[++current_output_index]  = '0'+(myAddr%10);
  //source_addr
  uint8_t source    = ppacketReceived->frame[F_SRC];
  output[++current_output_index]  = ' ';
  output[++current_output_index]  = '0'+((source/100)%10);
  output[++current_output_index]  = '0'+((source/10)%10);
  output[++current_output_index]  = '0'+(source%10);
  //seqnum
  uint8_t seqnum    = ppacketReceived->frame[F_DATA_SEQ];
  output[++current_output_index]  = ' ';
  output[++current_output_index]  = '0'+((seqnum/100)%10);
  output[++current_output_index]  = '0'+((seqnum/10)%10);
  output[++current_output_index]  = '0'+(seqnum%10);
  output[++current_output_index]  = ' ';
  //temperatures
  uint8_t temp_counter, current_packet_index=F_DATA_TEMP_1a-1;
  for (temp_counter=0;temp_counter<11;temp_counter++) {
    uint8_t temp_lsB  = ppacketReceived->frame[++current_packet_index];
    uint8_t temp_msB  = ppacketReceived->frame[++current_packet_index];
    int temp = (int)temp_lsB + ((int)temp_msB<<8);
    if(temp<0) {
      output[++current_output_index] = '-';
      temp = temp * -1;
    }
    output[++current_output_index] = '0'+((temp/1000)%10);
    output[++current_output_index] = '0'+((temp/100)%10);
    output[++current_output_index] = '0'+((temp/10)%10);
    output[++current_output_index] = '.';
    output[++current_output_index] = '0'+(temp%10);
    output[++current_output_index] = '&';
  }
  output[current_output_index] = ' ';
  //battery
  uint8_t batt      = ppacketReceived->frame[F_DATA_BATT];
  output[++current_output_index] = '0'+(batt/10)%10;
  output[++current_output_index] = '.';
  output[++current_output_index] = '0'+(batt%10);
  //rssi last packet
  int8_t  rssi      = ppacketReceived->rxMetrics[0];
  output[++current_output_index] = ' ';
  if(rssi<0) {
    output[++current_output_index] = '-';
    rssi = rssi * -1;
  }
  output[++current_output_index] = '0'+((rssi/100)%10);
  output[++current_output_index] = '0'+((rssi/10)%10);
  output[++current_output_index] = '0'+(rssi%10);
  //neighbors
  uint8_t num_neighbors = ppacketReceived->frame[F_DATA_NUMNGH];
  output[++current_output_index] = ' ';
  output[++current_output_index] = '0'+((num_neighbors/10)%10);
  output[++current_output_index] = '0'+(num_neighbors%10);
  output[++current_output_index] = ':';
  uint8_t temp_id;
  int8_t temp_rssi;
  current_packet_index=F_DATA_NUMNGH;
  for (temp_counter=0;temp_counter<num_neighbors;temp_counter++) {
    temp_id = ppacketReceived->frame[++current_packet_index];
    output[++current_output_index] = '0'+((temp_id/100)%10);
    output[++current_output_index] = '0'+((temp_id/10)%10);
    output[++current_output_index] = '0'+((temp_id)%10);
    temp_rssi = ppacketReceived->frame[++current_packet_index];
    output[++current_output_index] = '@';
    if(temp_rssi<0) {
      output[++current_output_index] = '-';
      temp_rssi = temp_rssi * -1;
    }
    output[++current_output_index] = '0'+((temp_rssi/100)%10);
    output[++current_output_index] = '0'+((temp_rssi/10)%10);
    output[++current_output_index] = '0'+(temp_rssi%10);
    output[++current_output_index] = ',';
  }
  output[current_output_index] = ' ';
  //hops
  uint8_t num_hops = ppacketReceived->frame[++current_packet_index];
  output[++current_output_index] = '0'+((num_hops/100)%10);
  output[++current_output_index] = '0'+((num_hops/10)%10);
  output[++current_output_index] = '0'+((num_hops)%10);
  output[++current_output_index] = ':';
  for (temp_counter=0;temp_counter<num_hops+1;temp_counter++) {
    temp_id = ppacketReceived->frame[++current_packet_index];
    output[++current_output_index] = '0'+((temp_id/100)%10);
    output[++current_output_index] = '0'+((temp_id/10)%10);
    output[++current_output_index] = '0'+((temp_id)%10);
    output[++current_output_index] = ',';
  }
  TXString(output,current_output_index);
  TXString("\n",1);
#endif
}

void print_FIN(mrfiPacket_t * pinBuffer)
{
#ifdef PRINT_FIN
  
#endif
}