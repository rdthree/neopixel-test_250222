/**
 * @file esp32s3_onboard_LED.c
 * @brief Single-file driver for ESP32-S3 onboard NeoPixel LED
 * 
 * This file contains everything needed to drive the onboard LED.
 * It's organized in sections: config, types, and functions.
 * For a project this size, single file is fine and might be
 * easier to understand than multiple files.
 */

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
 
 /*********************************************
  * Configuration
  *********************************************/
 
 // Hardware settings
 #define LED_GPIO            48      // Onboard NeoPixel GPIO pin
 #define LED_COUNT           1       // Single onboard NeoPixel
 
 // Animation settings
 #define RAINBOW_SPEED       20      // Animation speed (ms per step)
 #define HUE_STEP           2       // Hue increment per step (degrees)
 
 // RMT settings (for LED timing)
 #define RMT_RESOLUTION_HZ  10000000 // 10MHz for precise timing
 #define RMT_MEM_BLOCKS     64      // Memory blocks for RMT peripheral
 
 static const char *TAG = "NeoPixel";
 
 /*********************************************
  * Types
  *********************************************/
 
 /**
  * @brief Everything needed to control the LED
  */
 typedef struct {
     rmt_channel_handle_t channel;   // RMT channel for LED control
     rmt_encoder_handle_t encoder;   // LED strip encoder
     uint8_t grb_data[3];           // Current LED color in GRB format
     uint16_t current_hue;          // Current hue value (0-359 degrees)
 } led_controller_t;
 
 /*********************************************
  * Function Declarations
  *********************************************/
 
 static void hsv_to_grb(uint16_t h, uint8_t s, uint8_t v, uint8_t *grb);
 static led_controller_t initialize_led_controller(void);
 static void update_led_color(led_controller_t *controller);
 
 /*********************************************
  * Function Implementations
  *********************************************/
 
 /**
  * @brief Converts HSV color to GRB format for NeoPixel
  * 
  * Pretty standard HSV->RGB conversion, but outputs in GRB
  * order because that's what WS2812 LEDs expect.
  */
 static void hsv_to_grb(uint16_t h, uint8_t s, uint8_t v, uint8_t *grb) {
     h %= 360;
     float S = s / 100.0f;
     float V = v / 100.0f;
     float C = V * S;
     float X = C * (1 - fabs(fmod(h / 60.0f, 2) - 1));
     float m = V - C;
     
     float r, g, b;
     
     // Convert hue to RGB
     if (h < 60) {
         r = C; g = X; b = 0;
     } else if (h < 120) {
         r = X; g = C; b = 0;
     } else if (h < 180) {
         r = 0; g = C; b = X;
     } else if (h < 240) {
         r = 0; g = X; b = C;
     } else if (h < 300) {
         r = X; g = 0; b = C;
     } else {
         r = C; g = 0; b = X;
     }
     
     // Convert to GRB (WS2812 expects GRB, not RGB)
     grb[0] = (g + m) * 255;
     grb[1] = (r + m) * 255;
     grb[2] = (b + m) * 255;
 }
 
 /**
  * @brief Sets up everything needed to control the LED
  * 
  * Creates and configures the RMT peripheral which we need
  * because WS2812 LEDs have very strict timing requirements.
  */
 static led_controller_t initialize_led_controller(void) {
     led_controller_t controller = {0};
     
     // Set up RMT channel
     rmt_tx_channel_config_t tx_chan_config = {
         .clk_src = RMT_CLK_SRC_DEFAULT,
         .gpio_num = LED_GPIO,
         .mem_block_symbols = RMT_MEM_BLOCKS,
         .resolution_hz = RMT_RESOLUTION_HZ,
         .trans_queue_depth = 4,
     };
     ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &controller.channel));
 
     // Set up LED encoder
     led_strip_encoder_config_t encoder_config = {
         .resolution = RMT_RESOLUTION_HZ,
     };
     ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&encoder_config, &controller.encoder));
     
     // Turn it on
     ESP_ERROR_CHECK(rmt_enable(controller.channel));
     
     return controller;
 }
 
 /**
  * @brief Updates LED color and handles debug output
  * 
  * This function:
  * 1. Converts current hue to LED color values
  * 2. Sends color to LED
  * 3. Prints debug info (every 10 steps)
  * 4. Updates hue for next time
  */
 static void update_led_color(led_controller_t *controller) {
     // Convert hue to LED color
     hsv_to_grb(controller->current_hue, 100, 100, controller->grb_data);
     
     // Send to LED
     rmt_transmit_config_t tx_config = {
         .loop_count = 0
     };
     ESP_ERROR_CHECK(rmt_transmit(
         controller->channel, 
         controller->encoder,
         controller->grb_data, 
         sizeof(controller->grb_data),
         &tx_config
     ));
     
     // Debug output every 10 steps
     if (controller->current_hue % 10 == 0) {
         ESP_LOGI(TAG, "Hue: %d° | GRB: [%3d, %3d, %3d]", 
                 controller->current_hue,
                 controller->grb_data[0],
                 controller->grb_data[1],
                 controller->grb_data[2]);
     }
     
     // Move to next hue
     controller->current_hue = (controller->current_hue + HUE_STEP) % 360;
 }
 
 /**
  * @brief Main program entry
  * 
  * Sets up LED control and runs the animation forever.
  */
 void app_main(void) {
     ESP_LOGI(TAG, "Starting Rainbow Demo");
     
     led_controller_t controller = initialize_led_controller();
     
     // Just keep updating colors forever
     while (1) {
         update_led_color(&controller);
         vTaskDelay(pdMS_TO_TICKS(RAINBOW_SPEED));
     }
 }