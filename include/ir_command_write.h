
#ifndef IR_COMMAND_WRITE_H
#define IR_COMMAND_WRITE_H
#define TAG "IR_SNIFFER_WRITE"
#include "esp_log.h"
#include "driver/rmt_tx.h"

#define IR_LENGTH 34

typedef struct
{
    uint16_t duration0;
    uint16_t duration1;
} ir_symbol_t;

void write_command(ir_symbol_t to[5][IR_LENGTH],
                   const rmt_symbol_word_t from[],
                   rmt_symbol_word_t tx_symbols[],
                   int command_lengths[5],
                   bool *captured,
                   const int ir_length,
                   int cmd_index);

#endif