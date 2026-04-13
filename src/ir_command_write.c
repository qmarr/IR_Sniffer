#include "ir_command_write.h"

void write_command(ir_symbol_t to[5][IR_LENGTH],
                   const rmt_symbol_word_t from[],
                   rmt_symbol_word_t tx_symbols[],
                   int command_lengths[5],
                   bool *captured,
                   const int ir_length,
                   int cmd_index)
{

    for (size_t j = 0; j < ir_length; j++)
    {
        rmt_symbol_word_t s = from[j];
        to[cmd_index][j].duration0 = s.duration0;
        to[cmd_index][j].duration1 = s.duration1;
        ESP_LOGI(TAG,
                 "[%d] l0=%d d0=%d | l1=%d d1=%d",
                 j,
                 s.level0, s.duration0,
                 s.level1, s.duration1);
    }

    command_lengths[cmd_index] = ir_length;
    *captured = true;
    ESP_LOGI(TAG, "Command[%d] captured: %d symbols", cmd_index, command_lengths[cmd_index]);

    for (size_t j = 0; j < ir_length; j++)
    {
        tx_symbols[j].level0 = 1;
        tx_symbols[j].duration0 = to[cmd_index][j].duration0;
        tx_symbols[j].level1 = 0;
        tx_symbols[j].duration1 = to[cmd_index][j].duration1;
    }

    ESP_LOGI(TAG, "Command[%d] copied to tx_symbols", cmd_index);
}
