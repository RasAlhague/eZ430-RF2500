#ifndef GRADIENT_H
#define GRADIENT_H

//========================= main parameters ====================================
#define IS_SINK_NODE 0
//main times
#define WAKEUP_PERIOD_S_FIXED   3    //in s (WAKEUP_PERIOD_S_FIXED+WAKEUP_PERIOD_S_RANDOM<46)
#define WAKEUP_PERIOD_S_RANDOM  4    //in s (WAKEUP_PERIOD_S_FIXED+WAKEUP_PERIOD_S_RANDOM<46)
#define SEND_PERIOD             2    //in number of WAKE_UP periods
#define CHECK_INTERVAL          100  //in ms.
#define UF_PERIOD               2    //in ms.
#define CW_LENGTH               60   //in ms. TODO: calculate as a function of neighbors
//timeouts (>guard time)
#define TIMEOUT_CW              60   //in ms. If CW not received, abandon RX
#define TIMEOUT_DATA            60   //in ms. If DATA not received, abandon RX
#define TIMEOUT_FIN             60   //in ms. If FIN not received, consider TX failed
//guard times
#define GUARD_CW                5    //in ms. Early wake-up to receive CW
#define GUARD_DATA              5    //in ms. Early wake-up to receive DATA
#define GUARD_NUMBER_UF         3    //number of uF sent too much
#define DELAY_FIN               3    //in ms. delay between DATA received and FIN sent
#define ACK_DURATION            2    //in ms. maximum duration of an ACK message
//queueing
#define SIZE_Q                  3    //maximum number of elements in the Rx/Tx queues
#define MAX_RETRIES             3    //maximum number of times a queued packet is sent before dropped
//misc.
#define MAX_NEIGHBORS          10
/*0x00:-55dBm, 0x50:-30dBm, 0x44:-28dBm, 0xC0:-26dBm, 0x84:-24dBm, 0x81:-22dBm,
  0x46:-20dBm, 0x93:-18dBm, 0x55:-16dBm, 0x8D:-14dBm, 0xC6:-12dBm, 0x97:-10dBm,
  0x6E: -8dBm, 0x7F: -6dBm, 0xA9: -4dBm, 0xBB: -2dBm, 0xFE:  0dBm, 0xFF:  1dBm)*/
#define TX_POWER              0x55
//==============================================================================

#define NUMBER_UF             (CHECK_INTERVAL/UF_PERIOD)+GUARD_NUMBER_UF

//gradient protocol states
#define GRADIENT_IDLE        0
#define GRADIENT_TXUF        1
#define GRADIENT_TXCW        2
#define GRADIENT_RXACK       3
#define GRADIENT_TXDATA      4
#define GRADIENT_RXFIN       5
#define GRADIENT_RXUF        6
#define GRADIENT_RXCW        7
#define GRADIENT_BACKOFFACK  8
#define GRADIENT_TXACK       9
#define GRADIENT_SENTACK     10
#define GRADIENT_RXDATA      11
#define GRADIENT_TXFIN       12

//=============================== frames
/* frame types */
#define F_TYPE_UF         1
#define F_TYPE_CW         2
#define F_TYPE_ACK        3
#define F_TYPE_DATA       4
#define F_TYPE_FIN        5

/* Frame fields (in bytes, wrt the mrfi payload); frame: length (1B) ; source (4B) ; mrfi payload */
#define F_SRC             4
#define F_TYPE            9
//microframe (UF)
#define F_UF_COUNTER      10
#define F_UF_LENGTH       11
//new contention window (CW)
#define F_CW_CWDURATION   10
#define F_CW_LENGTH       11
//acknowledgment (ACK)
#define F_ACK_NEXT_HOP    10
#define F_ACK_HEIGHT      11
#define F_ACK_LENGTH      12
//data (DATA)
#define F_DATA_NEXT_HOP   8
#define F_DATA_SEQ        10
#define F_DATA_TEMP_1a    11
#define F_DATA_TEMP_1b    12
#define F_DATA_TEMP_2a    13
#define F_DATA_TEMP_2b    14
#define F_DATA_TEMP_3a    15
#define F_DATA_TEMP_3b    16
#define F_DATA_TEMP_4a    17
#define F_DATA_TEMP_4b    18
#define F_DATA_TEMP_5a    19
#define F_DATA_TEMP_5b    20
#define F_DATA_TEMP_6a    21
#define F_DATA_TEMP_6b    22
#define F_DATA_TEMP_7a    23
#define F_DATA_TEMP_7b    24
#define F_DATA_TEMP_8a    25
#define F_DATA_TEMP_8b    26
#define F_DATA_TEMP_9a    27
#define F_DATA_TEMP_9b    28
#define F_DATA_TEMP_10a   29
#define F_DATA_TEMP_10b   30
#define F_DATA_TEMP_11a   31
#define F_DATA_TEMP_11b   32
#define F_DATA_BATT       33
#define F_DATA_NUMNGH     34
//final acknowledgment (FIN)
#define F_FIN_NEXT_HOP    8
#define F_FIN_LENGTH      16

#define TOGGLE_EXT_PIN4()   (P2OUT ^= 0x02)
#define TOGGLE_EXT_PIN6()   (P2OUT ^= 0x08)
#define TOGGLE_EXT_PIN8()   (P4OUT ^= 0x08)
#define TOGGLE_EXT_PIN10()  (P4OUT ^= 0x20)

//=============================== neighbor table

typedef struct
{
  uint8_t  addr;
  uint8_t  height;
  uint8_t  rssi;
} neighbor_t;

//================================ prototypes
void          gradient_Init(uint8_t is_sink);
void          gradient_wakeup_timer();
void          gradient_wakeup_rx();
void          gradient_generate_data();
void          gradient_send_data();

#endif