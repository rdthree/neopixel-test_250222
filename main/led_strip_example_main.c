#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/rmt_tx.h"
#include "led_strip_encoder.h"
#include "esp_log.h"

#define LED_GPIO 48
#define LED_COUNT 1
#define RAINBOW_SPEED 20  // Lower = faster (ms per step)
#define HUE_STEP 2        // Degrees per step (0-360)

static const char *TAG = "NeoPixel";

// HSV to GRB conversion (h: 0-360, s/v: 0-100)
void hsv_to_grb(uint16_t h, uint8_t s, uint8_t v, uint8_t *grb) {
    float r, g, b;
    h %= 360;
    float S = s / 100.0f;
    float V = v / 100.0f;
    float C = V * S;
    float X = C * (1 - fabs(fmod(h / 60.0f, 2) - 1));
    float m = V - C;

    if(h < 60) {
        r = C; g = X; b = 0;
    } else if(h < 120) {
        r = X; g = C; b = 0;
    } else if(h < 180) {
        r = 0; g = C; b = X;
    } else if(h < 240) {
        r = 0; g = X; b = C;
    } else if(h < 300) {
        r = X; g = 0; b = C;
    } else {
        r = C; g = 0; b = X;
    }
    
    grb[0] = (g + m) * 255;  // Green
    grb[1] = (r + m) * 255;  // Red
    grb[2] = (b + m) * 255;  // Blue
}

void app_main(void) {
    ESP_LOGI(TAG, "Starting Rainbow Demo");
    
    // RMT Initialization (keep your existing setup)
    rmt_channel_handle_t led_chan = NULL;
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = LED_GPIO,
        .mem_block_symbols = 64,
        .resolution_hz = 10 * 1000 * 1000,
        .trans_queue_depth = 4,
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &led_chan));

    rmt_encoder_handle_t led_encoder = NULL;
    led_strip_encoder_config_t encoder_config = {
        .resolution = 10 * 1000 * 1000,
    };
    ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&encoder_config, &led_encoder));
    ESP_ERROR_CHECK(rmt_enable(led_chan));

    uint8_t grb_data[3] = {0};
    rmt_transmit_config_t tx_config = {.loop_count = 0};
    uint16_t hue = 0;

    while(1) {
        // Generate rainbow colors
        hsv_to_grb(hue, 100, 100, grb_data);
        
        // Send to LED
        ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, grb_data, sizeof(grb_data), &tx_config));
        
        // Print color info every 10 steps
        if(hue % 10 == 0) {
            ESP_LOGI(TAG, "Hue: %d° | GRB: [%3d, %3d, %3d]", 
                    hue, grb_data[0], grb_data[1], grb_data[2]);
        }

        // Update hue and delay
        hue = (hue + HUE_STEP) % 360;
        vTaskDelay(pdMS_TO_TICKS(RAINBOW_SPEED));
    }
}