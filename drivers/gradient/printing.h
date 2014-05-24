#ifndef PRINTING_H
#define PRINTING_H

//#define PRINT_SUCCESS
//#define PRINT_FAIL
//#define PRINT_DEBUG
//#define PRINT_HEIGHT
//#define PRINT_SINK
//#define PRINT_NEIGHBOR_TABLE
//#define PRINT_Q
//#define PRINT_INTERRUPTS
//#define PRINT_GRADIENT_STATE
//#define PRINT_CC2500_STATUS
//#define PRINT_CC2500_REGISTERS
//#define PRINT_CC2500_WOR_STATUS
//#define PRINT_CC2500_WOR_REGISTERS

#define PRINT_UF
#define PRINT_CW
#define PRINT_ACK
#define PRINT_DATA
#define PRINT_FIN

void print_debug(char*, uint8_t);
void print_fail(char*, uint8_t);
void print_success(char*, uint8_t);
void print_interrupt(char*, uint8_t);
void print_height(uint8_t, uint8_t);
void print_neighbor_table(neighbor_t*);
void print_gradient_state(uint8_t);
void print_cc2500_wor_status(char*, uint8_t);
void print_cc2500_status();
void print_cc2500_registers();
void print_cc2500_wor_registers();

void print_UF(mrfiPacket_t*);
void print_CW(mrfiPacket_t*);
void print_ACK(mrfiPacket_t*);
void print_DATA(mrfiPacket_t * ppacketReceived, uint8_t myAddr);
void print_FIN(mrfiPacket_t*);

#endif