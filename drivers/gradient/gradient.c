#include "mrfi.h"
#include "wor.h"
#include "gradient.h"
#include "bsp_leds.h"
#include "sensing.h"
#include "radios/family1/mrfi_spi.h"
#include "printing.h"
#include "mrfi_defs.h"
#include "QMgmt.h"

static uint8_t      myAddr;
static uint8_t      myHeight;
static uint8_t      gradient_state;
static neighbor_t   neighbors[MAX_NEIGHBORS];
static uint8_t      seq=0;
static frameInfo_t* dataToSend;
static mrfiPacket_t lastData;
static uint8_t      uf_counter;
static uint8_t      addr_initiator;
__no_init volatile uint8_t Flash_Addr @ 0x10F0;

void    gradient_end_txrx();
void    gradient_set_state(uint8_t new_state);
void    gradient_init_neighbor_table();
void    gradient_add_neighbor(mrfiPacket_t * pinBuffer);
void    gradient_update_height();
uint8_t gradient_next_hop();
void    set_timer(uint16_t timeout_in_ms);
void    set_random_timer(uint16_t max_value_ms);
void    set_second_random_timer(uint16_t max_value_ms);
void    stop_timer();
void    stop_second_random_timer();
void    gradient_build_UF(mrfiPacket_t*, uint8_t);
void    gradient_build_CW(mrfiPacket_t*, uint8_t);
void    gradient_build_ACK(mrfiPacket_t*, uint8_t);
void    gradient_build_DATA_payload(mrfiPacket_t* ppacket, uint8_t source, uint8_t seqnum, uint16_t *temperatures, uint8_t batt);
void    gradient_build_DATA_headers(frameInfo_t* frame);
void    gradient_build_DATA_path(mrfiPacket_t* packet);
void    gradient_build_FIN(mrfiPacket_t*, uint8_t);

//################################### public functions

//=============================================
void gradient_Init(uint8_t is_sink)
{
  P1OUT &= ~0x03;
  print_debug("\r\nRESTART",9);
  //serial communication
  P3SEL    |= 0x30;     // P3.4,5 = USCI_A0 TXD/RXD
  UCA0CTL1  = UCSSEL_2; // SMCLK
  UCA0BR0   = 0x41;     // 9600 from 8Mhz
  UCA0BR1   = 0x3;
  UCA0MCTL  = UCBRS_2;                     
  UCA0CTL1 &= ~UCSWRST; // Initialize USCI state machine
  IE2      |= UCA0RXIE; // Enable USCI_A0 RX interrupt
  __enable_interrupt();
  //button
  P1DIR    &= ~0x04;                        // P1.3 input
  P1OUT     = 0x04;
  P1REN    |= 0x04;                         // P1.3 pullup
  P1IE     |= 0x04;                         // P1.3 interrupt enabled
  P1IES    |= 0x04;                         // P1.3 Hi/lo edge
  P1IFG    &= ~0x04;                        // P1.3 IFG cleared
  //extension pins used for debugging
  P4SEL    &= ~0x20;                        // P4.5 generic I/O pin
  P2DIR    |=  0x02;                        // P2.1 output @ extension pin P4
  P2DIR    |=  0x08;                        // P2.3 output @ extension pin P6
  P4DIR    |=  0x08;                        // P4.3 output @ extension pin P8
  P4DIR    |=  0x20;                        // P4.5 output @ extension pin P10
  //set myAddr
  if(Flash_Addr == 0xFF)                      // no address set before
  {
    myAddr = MRFI_RandomByte();
    FCTL2 = FWKEY + FSSEL0 + FN1;             // MCLK/3 for Flash Timing Generator
    FCTL3 = FWKEY + LOCKA;                    // Clear LOCK & LOCKA bits
    FCTL1 = FWKEY + WRT;                      // Set WRT bit for write operation
    Flash_Addr = myAddr;
    FCTL1 = FWKEY;                            // Clear WRT bit
    FCTL3 = FWKEY + LOCKA + LOCK;             // Set LOCK & LOCKA bit
  } else {
    myAddr=Flash_Addr;
  }
  //set height and start clock
  if (is_sink) {
    myHeight = 0;
    BSP_TOGGLE_LED1();
  } else {
    myHeight = 255;
    //timer A
    BCSCTL3  |= LFXT1S_2;                      // LFXT1 = VLO
    TACCTL0   = CCIE;                          // TACCR0 interrupt enabled
    TACCR0    = 1500*WAKEUP_PERIOD_S_FIXED;    // 1500=1 second
    TACTL     = TASSEL_1 + MC_1 + ID_3;        // ACLK/8, upmode
  }
  gradient_set_state(GRADIENT_IDLE);
  mrfiSpiWriteReg(PATABLE, TX_POWER);
  print_cc2500_registers();
  MRFI_WakeUp();
  wor_start(IS_SINK_NODE);
}

//=============================================
void gradient_wakeup_timer()
{
  uint8_t next_hop_addr;
  mrfiPacket_t packetToSend;
  switch (gradient_state) {
  //--------------------------------------- transmitter
  case (GRADIENT_IDLE):                                   //wake_up timer
    /* stop preamble sampling */
    wor_stop(IS_SINK_NODE);
    /* stop possible timers */
    stop_timer();
    /* check that there is data to send */
    dataToSend = QgetInUseSlot(OUTQ);
    if (!dataToSend) {
      gradient_end_txrx();
      print_debug("\r\nno data to send",17);
      return;
    }
    /* TODO. check that the medium is free */
    set_timer(UF_PERIOD);                                  //wake_up timer
    /* send first uF */
    gradient_set_state(GRADIENT_TXUF);
    uf_counter = NUMBER_UF-1;
    gradient_build_UF(&packetToSend, uf_counter);
    MRFI_justTransmit(&packetToSend);
    break;
  case (GRADIENT_TXUF):                                   //wake_up timer
    if (uf_counter > 0) {
      uf_counter--;
      /* send uF */
      gradient_build_UF(&packetToSend, uf_counter);
      MRFI_justTransmit(&packetToSend);
    } else {
      stop_timer();
      /* send CW */
      gradient_set_state(GRADIENT_TXCW);
      gradient_build_CW(&packetToSend, CW_LENGTH);
      MRFI_justTransmit(&packetToSend);
      /* wait for ACK */
      gradient_set_state(GRADIENT_RXACK);
      gradient_init_neighbor_table();
      MRFI_RxOn();
      set_timer(CW_LENGTH);
    }
    break;
  case (GRADIENT_RXACK):                                  //wake_up timer
    stop_timer();
    print_neighbor_table(neighbors);
    gradient_update_height();
    print_height(myAddr,myHeight);
    next_hop_addr = gradient_next_hop();
    if (next_hop_addr==0) {
      gradient_end_txrx();
      print_fail("\r\nTX failed: no neighbor",24);
    } else {
      /* send DATA */
      gradient_set_state(GRADIENT_TXDATA);
      //if mine, complete with current neighbor list and empty hop count
      if ( (&(dataToSend->mrfiPkt))->frame[F_SRC]==myAddr) {
        gradient_build_DATA_headers(dataToSend);
      }
      (&(dataToSend->mrfiPkt))->frame[F_DATA_NEXT_HOP]=next_hop_addr;
      MRFI_justTransmit(&(dataToSend->mrfiPkt));
      /* wait for FIN */
      gradient_set_state(GRADIENT_RXFIN);
      MRFI_RxOn();
      set_timer(TIMEOUT_FIN);
    }
    break;
  case (GRADIENT_RXFIN):                                  //wake_up timer
    gradient_end_txrx();
    print_fail("\r\nTX failed: no FIN",19);
    break;
  //--------------------------------------- receiver
  case (GRADIENT_RXUF):                                   //wake_up timer
    stop_timer();
    /* wait for CW */
    gradient_set_state(GRADIENT_RXCW);
    MRFI_RxOn();
    set_timer(TIMEOUT_CW);
    break;
  case (GRADIENT_RXCW):                                   //wake_up timer
    gradient_end_txrx();
    print_fail("\r\nRX failed: no CW",18);
    break;
  case (GRADIENT_BACKOFFACK):                             //wake_up timer
    stop_second_random_timer();
    /* send ACK */
    gradient_set_state(GRADIENT_TXACK);
    gradient_build_ACK(&packetToSend, addr_initiator);
    MRFI_justTransmit(&packetToSend);
    gradient_set_state(GRADIENT_SENTACK);
    MRFI_RxIdle();
    break;
  case (GRADIENT_SENTACK):                                //wake_up timer
    stop_timer();
    /* wait for DATA */
    gradient_set_state(GRADIENT_RXDATA);
    MRFI_RxOn();
    set_timer(TIMEOUT_DATA);
    break;
  case (GRADIENT_RXDATA):                                 //wake_up timer
    gradient_end_txrx();
    print_fail("\r\nRX failed: no DATA",20);
    break;
  case (GRADIENT_TXFIN):                                  //wake_up timer
    stop_timer();
    /* send FIN */
    gradient_build_FIN(&packetToSend, addr_initiator);
    MRFI_justTransmit(&packetToSend);
    print_success("\r\nRX successful",15);
    print_DATA(&lastData, myAddr);
    gradient_end_txrx();
    break;
  }
}

//=============================================
void gradient_wakeup_rx()
{
  mrfiPacket_t packetReceived;
  frameInfo_t * freeSlot;
  MRFI_Receive(&packetReceived);
  uint8_t pkt_next_hop;
  uint8_t pkt_type = packetReceived.frame[F_TYPE];
  switch (gradient_state) {
  //--------------------------------------- transmitter
  case (GRADIENT_RXACK):                                  //rx
    pkt_next_hop = packetReceived.frame[F_ACK_NEXT_HOP];
    if (pkt_type==F_TYPE_ACK && pkt_next_hop==myAddr) {
      gradient_add_neighbor(&packetReceived);
    }
    break;
  case (GRADIENT_RXFIN):                                  //rx
    pkt_next_hop = packetReceived.frame[F_FIN_NEXT_HOP];
    if (pkt_type==F_TYPE_FIN && pkt_next_hop==myAddr) {
      stop_timer();
      print_success("\r\nTX successful",15);
      QfreeSlot(dataToSend);
      gradient_end_txrx();
    }
    break;
  //--------------------------------------- receiver
  case (GRADIENT_IDLE):                                   //rx
    if (pkt_type==F_TYPE_UF) {
      /* stop preamble sampling */
      wor_stop(IS_SINK_NODE);
      /* stop possible timers (if OUTQ not empty) */
      stop_timer();
      gradient_set_state(GRADIENT_RXUF);
      addr_initiator = packetReceived.frame[F_SRC];
      MRFI_RxIdle();
      set_timer((packetReceived.frame[F_UF_COUNTER])*UF_PERIOD-GUARD_CW);
    }
    break;
  case (GRADIENT_RXCW):                                   //rx
    if (pkt_type==F_TYPE_CW) {
      stop_timer();
      gradient_set_state(GRADIENT_BACKOFFACK);
      MRFI_RxIdle();
      /* second timer to send my ACK */
      set_second_random_timer((packetReceived.frame[F_CW_CWDURATION])-GUARD_DATA-ACK_DURATION);
      /* main timer at the end of CW */
      set_timer((packetReceived.frame[F_CW_CWDURATION])-GUARD_DATA);
    } else if (pkt_type==F_TYPE_UF) {
      set_timer(TIMEOUT_CW);
    }
    break;
  case (GRADIENT_RXDATA):                                 //rx
    pkt_next_hop = packetReceived.frame[F_DATA_NEXT_HOP];
    if (pkt_type==F_TYPE_DATA && pkt_next_hop==myAddr) {
      stop_timer();
      gradient_build_DATA_path(&packetReceived); //add myAddr to packet
      if (myHeight!=0) { //I'm not a sink
        /* add packet to outQ for later relaying if not sink */
        print_debug("\r\nQueued for relay",18);
        freeSlot = QfindFreeSlot(OUTQ);
        if (freeSlot) {
          freeSlot->mrfiPkt = packetReceived;
        }
      } else {       //I'm a sink
        /* process incoming packet through INQ */
        frameInfo_t * freeSlot = QfindFreeSlot(INQ);
        if (freeSlot) {
          freeSlot->mrfiPkt = packetReceived;
          lastData = packetReceived;
          QfreeSlot(freeSlot);
        }
      }
      /* delay before sending FIN */      
      gradient_set_state(GRADIENT_TXFIN);
      MRFI_RxIdle();
      set_timer(DELAY_FIN);
    }
    break;
  }
}

//=============================================
void gradient_generate_data() {
  uint16_t temperatures[11];
  uint8_t battery;
  frameInfo_t * freeSlot = QfindFreeSlot(OUTQ);
  if (freeSlot) {
    //get_adc_temperatures(&(temperatures[1]));
    temperatures[0]  = get_msp_temperature();   
    temperatures[1]  = get_adc_temperatures(15);
    temperatures[2]  = get_adc_temperatures(14);
    temperatures[3]  = get_adc_temperatures(13);
    temperatures[4]  = get_adc_temperatures(12);
    temperatures[5]  = get_adc_temperatures(5);
    temperatures[6]  = get_adc_temperatures(4);
    temperatures[7]  = get_adc_temperatures(3);
    temperatures[8]  = get_adc_temperatures(2);
    temperatures[9]  = get_adc_temperatures(1);
    temperatures[10] = get_adc_temperatures(0);
    battery          = get_battery();
    gradient_build_DATA_payload(&(freeSlot->mrfiPkt), myAddr, seq, temperatures, battery);
    seq++;
  }
}

//=============================================
void gradient_send_data() {
  if (gradient_state==GRADIENT_IDLE) {
    gradient_wakeup_timer();
  }
}

//################################### helper functions

void gradient_end_txrx()
{
  stop_timer();
  Qprint();
  gradient_set_state(GRADIENT_IDLE);
  wor_start(IS_SINK_NODE);
}

void gradient_set_state(uint8_t new_state)
{
  gradient_state = new_state;
  print_gradient_state(new_state);
}

//========================= neighbor table
void gradient_init_neighbor_table()
{
  uint8_t i;
  for (i=0; i<MAX_NEIGHBORS; i++) {
    neighbors[i].addr   = 0;
    neighbors[i].height = 255;
    neighbors[i].rssi   = 0;
  }
}

void gradient_add_neighbor(mrfiPacket_t * ppacketReceived)
{
  uint8_t i=0, addr, height, rssi;
  addr   = ppacketReceived->frame[F_SRC];
  height = ppacketReceived->frame[F_ACK_HEIGHT];
  rssi   = ppacketReceived->rxMetrics[0];
  while (i<MAX_NEIGHBORS) {
    if (neighbors[i].addr==0 || neighbors[i].addr==addr) {
      neighbors[i].addr   = addr;
      neighbors[i].height = height;
      neighbors[i].rssi   = rssi;
      break;
    }
    i++;
  }
}

//========================= height (routing)
void gradient_update_height()
{
  uint8_t i=0;
  if (myHeight!=0) { //only update when I'm not a sink
    myHeight=255;
    for (i=0; i<MAX_NEIGHBORS; i++) {
      if (neighbors[i].addr!=0 && neighbors[i].height+1<myHeight) {
        myHeight=neighbors[i].height+1;
      }
    }
  }
}

uint8_t gradient_next_hop()
{
  uint8_t i=0, next_hop_addr=0, next_hop_height=255;
  for (i=0; i<MAX_NEIGHBORS; i++) {
    if (neighbors[i].addr!=0 && neighbors[i].height<=next_hop_height) {
      next_hop_height = neighbors[i].height;
      next_hop_addr   = neighbors[i].addr;
    }
  }
  return next_hop_addr;
}

//========================= timers
void set_timer(uint16_t timeout_in_ms)
{
  TBCTL  |= TBCLR;                          // clear the timer 
  TBCCTL0 = CCIE;                           // TBCCR0 interrupt enabled
  TBCCR0  = timeout_in_ms*1000;             // measured 1000 = 1 second (8MHz SMCLK)
  TBCTL   = TBSSEL_2 + 0x00C0 + MC_1;       // source from SMCLK/8, upmode
}

void set_random_timer(uint16_t max_value_ms)
{
  uint16_t max_value, random_value, random_less_than_max;
  max_value            = max_value_ms*1000;   // measured 1000 = 1 second (8MHz SMCLK)
  random_value         = 0;
  random_value        |= MRFI_RandomByte();
  random_value        |= (MRFI_RandomByte()<<8);
  random_less_than_max = random_value%max_value;
  TBCCTL0 = CCIE;                             // TBCCR0 interrupt enabled
  TBCCR0  = random_less_than_max;           
  TBCTL   = TBSSEL_2 + 0x00C0 + MC_1;         // source from SMCLK/8, upmode
}

/* this timer needs to be smaller than the first */
void set_second_random_timer(uint16_t max_value_ms)
{
  uint16_t max_value, random_value, random_less_than_max;
  max_value            = max_value_ms*1000;    // measured 1000 = 1 second (8MHz SMCLK)
  random_value         = 0;
  random_value        |= MRFI_RandomByte();
  random_value        |= (MRFI_RandomByte()<<8);
  random_less_than_max = random_value%max_value;
  TBCCTL1 = CCIE;                             // TBCCR1 interrupt enabled
  TBCCR1  = random_less_than_max;           
  TBCTL  |= TBCLR;                            // clear the timer
}

void stop_timer()
{
  TBCCTL0  = 0;                               // disable interrupts
  TBCTL    = MC_0;                            // stop Timer_B
}

void stop_second_random_timer()
{
  TBCCTL1 = 0;
  TBCCTL2 = 0;
}

//=============================================
void gradient_build_UF(mrfiPacket_t* ppacket, uint8_t counter)
{
  ppacket->frame[0]               = F_UF_LENGTH;
  ppacket->frame[1]               = 0x00;
  ppacket->frame[2]               = 0x00;
  ppacket->frame[3]               = 0x00;
  ppacket->frame[F_SRC]           = myAddr;
  ppacket->frame[5]               = 0xFF;
  ppacket->frame[6]               = 0xFF;
  ppacket->frame[7]               = 0xFF;
  ppacket->frame[8]               = 0xFF;
  ppacket->frame[F_TYPE]          = F_TYPE_UF;
  ppacket->frame[F_UF_COUNTER]    = counter;
}

void gradient_build_CW(mrfiPacket_t* ppacket, uint8_t cwduration)
{
  ppacket->frame[0]               = F_CW_LENGTH;
  ppacket->frame[1]               = 0x00;
  ppacket->frame[2]               = 0x00;
  ppacket->frame[3]               = 0x00;
  ppacket->frame[F_SRC]           = myAddr;
  ppacket->frame[5]               = 0xFF;
  ppacket->frame[6]               = 0xFF;
  ppacket->frame[7]               = 0xFF;
  ppacket->frame[8]               = 0xFF;
  ppacket->frame[F_TYPE]          = F_TYPE_CW;
  ppacket->frame[F_CW_CWDURATION]  = cwduration;
}

void gradient_build_ACK(mrfiPacket_t* ppacket, uint8_t next_hop)
{
  ppacket->frame[0]               = F_ACK_LENGTH;
  ppacket->frame[1]               = 0x00;
  ppacket->frame[2]               = 0x00;
  ppacket->frame[3]               = 0x00;
  ppacket->frame[F_SRC]           = myAddr;
  ppacket->frame[5]               = 0xFF;
  ppacket->frame[6]               = 0xFF;
  ppacket->frame[7]               = 0xFF;
  ppacket->frame[8]               = 0xFF;
  ppacket->frame[F_TYPE]          = F_TYPE_ACK;
  ppacket->frame[F_ACK_NEXT_HOP]  = next_hop;
  ppacket->frame[F_ACK_HEIGHT]    = myHeight;
}

void gradient_build_DATA_payload(mrfiPacket_t* ppacket, uint8_t source, uint8_t seqnum, uint16_t *temp, uint8_t batt)
{
  ppacket->frame[1]                = 0x00;
  ppacket->frame[2]                = 0x00;
  ppacket->frame[3]                = 0x00;
  ppacket->frame[F_SRC]            = source;
  ppacket->frame[5]                = 0x00;
  ppacket->frame[6]                = 0x00;
  ppacket->frame[7]                = 0x00;
  ppacket->frame[F_DATA_NEXT_HOP]  = 0x00; //filled in later on
  ppacket->frame[F_TYPE]           = F_TYPE_DATA;
  ppacket->frame[F_DATA_SEQ]       = seqnum;
  ppacket->frame[F_DATA_TEMP_1a]   = temp[0]&0xFF;
  ppacket->frame[F_DATA_TEMP_1b]   = (temp[0]>>8)&0xFF;
  ppacket->frame[F_DATA_TEMP_2a]   = temp[1]&0xFF;
  ppacket->frame[F_DATA_TEMP_2b]   = (temp[1]>>8)&0xFF;
  ppacket->frame[F_DATA_TEMP_3a]   = temp[2]&0xFF;
  ppacket->frame[F_DATA_TEMP_3b]   = (temp[2]>>8)&0xFF;
  ppacket->frame[F_DATA_TEMP_4a]   = temp[3]&0xFF;
  ppacket->frame[F_DATA_TEMP_4b]   = (temp[3]>>8)&0xFF;
  ppacket->frame[F_DATA_TEMP_5a]   = temp[4]&0xFF;
  ppacket->frame[F_DATA_TEMP_5b]   = (temp[4]>>8)&0xFF;
  ppacket->frame[F_DATA_TEMP_6a]   = temp[5]&0xFF;
  ppacket->frame[F_DATA_TEMP_6b]   = (temp[5]>>8)&0xFF;
  ppacket->frame[F_DATA_TEMP_7a]   = temp[6]&0xFF;
  ppacket->frame[F_DATA_TEMP_7b]   = (temp[6]>>8)&0xFF;
  ppacket->frame[F_DATA_TEMP_8a]   = temp[7]&0xFF;
  ppacket->frame[F_DATA_TEMP_8b]   = (temp[7]>>8)&0xFF;
  ppacket->frame[F_DATA_TEMP_9a]   = temp[8]&0xFF;
  ppacket->frame[F_DATA_TEMP_9b]   = (temp[8]>>8)&0xFF;
  ppacket->frame[F_DATA_TEMP_10a]  = temp[9]&0xFF;
  ppacket->frame[F_DATA_TEMP_10b]  = (temp[9]>>8)&0xFF;
  ppacket->frame[F_DATA_TEMP_11a]  = temp[10]&0xFF;
  ppacket->frame[F_DATA_TEMP_11b]  = (temp[10]>>8)&0xFF;
  ppacket->frame[F_DATA_BATT]      = batt;
  //this packet will be completed when sent (neighbors list and initial path)
}

void gradient_build_DATA_headers(frameInfo_t* frame)
{
  uint8_t i, neighbor_counter=0, current_pkt_index=F_DATA_NUMNGH;
  for (i=0; i<MAX_NEIGHBORS; i++) {
    if (neighbors[i].addr!=0) {
      (&(frame->mrfiPkt))->frame[++current_pkt_index] = neighbors[i].addr;
      (&(frame->mrfiPkt))->frame[++current_pkt_index] = neighbors[i].rssi;
      neighbor_counter++;
    }
  }
  (&(frame->mrfiPkt))->frame[F_DATA_NUMNGH]       = neighbor_counter;
  (&(frame->mrfiPkt))->frame[++current_pkt_index] = 0;
  (&(frame->mrfiPkt))->frame[++current_pkt_index] = myAddr;
  (&(frame->mrfiPkt))->frame[0]                   = current_pkt_index;
}

void gradient_build_DATA_path(mrfiPacket_t* packet)
{
  uint8_t temp_num_hops = packet->frame[F_DATA_NUMNGH+2*packet->frame[F_DATA_NUMNGH]+1];
  packet->frame[F_DATA_NUMNGH+2*packet->frame[F_DATA_NUMNGH]+1] = temp_num_hops+1;
  packet->frame[packet->frame[0]+1]                             = myAddr;      
  packet->frame[0]                                              = packet->frame[0]+1;
}

void gradient_build_FIN(mrfiPacket_t* ppacket, uint8_t next_hop)
{
  ppacket->frame[0]               = F_FIN_LENGTH;
  ppacket->frame[1]               = 0x00;
  ppacket->frame[2]               = 0x00;
  ppacket->frame[3]               = 0x00;
  ppacket->frame[F_SRC]           = myAddr;
  ppacket->frame[5]               = 0x00;
  ppacket->frame[6]               = 0x00;
  ppacket->frame[7]               = 0x00;
  ppacket->frame[F_FIN_NEXT_HOP]  = next_hop;
  ppacket->frame[F_TYPE]          = F_TYPE_FIN;
}