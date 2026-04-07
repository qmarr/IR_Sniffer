#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/rmt_rx.h"
#include "driver/gpio.h"

#define TAG "IR_SNIFFER"

#define RMT_RX_GPIO GPIO_NUM_15
#define IR_LED GPIO_NUM_16
#define RMT_RESOLUTION_HZ 1000000 // 1us per tick

#define MEM_BLOCK_SYMBOLS 64
#define QUEUE_SIZE 4

typedef struct
{
    uint16_t duration0;
    uint16_t duration1;
} ir_symbol_t;

static QueueHandle_t ir_queue;

// callback з ISR контексту
static bool ir_rx_done_callback(rmt_channel_handle_t channel,
                                const rmt_rx_done_event_data_t *edata,
                                void *user_data)
{
    BaseType_t high_task_wakeup = pdFALSE;

    QueueHandle_t queue = (QueueHandle_t)user_data;
    xQueueSendFromISR(queue, edata, &high_task_wakeup);

    return high_task_wakeup == pdTRUE;
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting IR sniffer...");

    ir_queue = xQueueCreate(QUEUE_SIZE, sizeof(rmt_rx_done_event_data_t));

    // 1. Конфіг RX каналу
    rmt_rx_channel_config_t rx_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = RMT_RESOLUTION_HZ,
        .mem_block_symbols = MEM_BLOCK_SYMBOLS,
        .gpio_num = RMT_RX_GPIO,
    };

    rmt_channel_handle_t rx_channel = NULL;
    ESP_ERROR_CHECK(rmt_new_rx_channel(&rx_config, &rx_channel));

    // 2. Callback
    rmt_rx_event_callbacks_t cbs = {
        .on_recv_done = ir_rx_done_callback,
    };

    ESP_ERROR_CHECK(rmt_rx_register_event_callbacks(rx_channel, &cbs, ir_queue));

    // 3. Enable channel
    ESP_ERROR_CHECK(rmt_enable(rx_channel));

    // 4. Буфер для прийому
    rmt_symbol_word_t raw_symbols[MEM_BLOCK_SYMBOLS];

    rmt_receive_config_t receive_config = {
        .signal_range_min_ns = 1000,     
        .signal_range_max_ns = 10000000, 
    };

    ESP_ERROR_CHECK(rmt_receive(rx_channel, raw_symbols,
                                sizeof(raw_symbols), &receive_config));


    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << IR_LED),
        .mode = GPIO_MODE_OUTPUT,
    };

    gpio_config(&io_conf);


    ir_symbol_t power_button[34];
    int power_button_len = 0;
    bool captured = false;
    while (1)
    {
        rmt_rx_done_event_data_t rx_data;

        gpio_set_level(IR_LED, true);
        vTaskDelay(1000/portTICK_PERIOD_MS);

        if (xQueueReceive(ir_queue, &rx_data, portMAX_DELAY))
        {
            int count = rx_data.num_symbols;
            ESP_LOGI(TAG, "Frame received: symbols = %d", rx_data.num_symbols);

            if (count > 34)
                count = 34;
            if (!captured)
            {
                for (int i = 0; i < count; i++)
                {
                    rmt_symbol_word_t s = raw_symbols[i];
                    power_button[i].duration0 = s.duration0;
                    power_button[i].duration1 = s.duration1;
                    ESP_LOGI(TAG,
                             "[%d] l0=%d d0=%d | l1=%d d1=%d",
                             i,
                             s.level0, s.duration0,
                             s.level1, s.duration1);
                }
                power_button_len = count;
                captured = true;
                ESP_LOGI(TAG, "Power button captured: %d symbols", power_button_len);
            }
            else
            {
                ESP_LOGI(TAG, "Frame ignored, already captured");
            }
            ESP_LOGI(TAG, "---- END FRAME ----");

            // важливо: знову запустити прийом
            ESP_ERROR_CHECK(rmt_receive(rx_channel, raw_symbols,
                                        sizeof(raw_symbols), &receive_config));
        }
    }
}