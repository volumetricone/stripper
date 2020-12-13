/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>

#define DEFAULT_POWER false
#define DEFAULT_BRIGHTNESS 50
#define DEFAULT_CCT 3000
#define DEFAULT_MIN_CCT 2700
#define DEFAULT_MAX_CCT 6500

#define DEFAULT_OTA_NAME "Update firmware"

#define BUTTON_ACTIVE_LEVEL 0

// Buttons
#define BUTTON_GPIO_0 4ULL  // Button 1
#define BUTTON_GPIO_1 25ULL // Button 2
#define BUTTON_GPIO_2 26ULL // Button 4
#define BUTTON_GPIO_3 27ULL // Button 5

// LEDC
#define LEDC_RESOLUTION LEDC_TIMER_10_BIT
#define LED_FREQUENCY 40000
#define LEDC_LEVELS 1023

// Outputs
#define OUTPUT_NUM 2
#define OUTPUT_GPIO_0 23ULL // Output 1
#define OUTPUT_GPIO_1 22ULL // Output 2
#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_MODE LEDC_HIGH_SPEED_MODE
#define OUTPUT_LEDC_CHANNEL_0 LEDC_CHANNEL_0
#define OUTPUT_LEDC_CHANNEL_1 LEDC_CHANNEL_1
#define WARM_OUTPUT 0
#define COLD_OUTPUT 1

#define WIFI_RESET_BUTTON_TIMEOUT 5
#define FACTORY_RESET_BUTTON_TIMEOUT 20

void app_driver_init(void);
esp_err_t light_set_cct_brightness(uint16_t cct, uint16_t brightness);
esp_err_t app_light_set_power(bool state);
esp_err_t app_light_set_cct(uint16_t cct);
esp_err_t app_light_set_brightness(uint16_t brightness);