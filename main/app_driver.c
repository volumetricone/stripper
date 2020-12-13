#include <sdkconfig.h>
#include <string.h>
#include <esp_log.h>
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <app_reset.h>
#include "app_priv.h"
#include "driver/ledc.h"

static bool g_power = DEFAULT_POWER;
static uint16_t g_cct = DEFAULT_CCT;
static uint16_t g_brightness = DEFAULT_BRIGHTNESS;

/*
     * Prepare and set configuration of timers
     * that will be used by LED Controller
     */
ledc_timer_config_t ledc_timer = {
    .duty_resolution = LEDC_RESOLUTION, // resolution of PWM duty
    .freq_hz = LED_FREQUENCY,           // frequency of PWM signal
    .speed_mode = LEDC_MODE,            // timer mode
    .timer_num = LEDC_TIMER,            // timer index
    .clk_cfg = LEDC_AUTO_CLK,           // Auto select the source clock
};

/*
     * Prepare individual configuration
     * for each channel of LED Controller
     * by selecting:
     * - controller's channel number
     * - output duty cycle, set initially to 0
     * - GPIO number where LED is connected to
     * - speed mode, either high or low
     * - timer servicing selected channel
     *   Note: if different channels use one timer,
     *         then frequency and bit_num of these channels
     *         will be the same
     */
ledc_channel_config_t ledc_channel[OUTPUT_NUM] = {
    {.channel = OUTPUT_LEDC_CHANNEL_0,
     .duty = 0,
     .gpio_num = OUTPUT_GPIO_0,
     .speed_mode = LEDC_MODE,
     .hpoint = 0,
     .timer_sel = LEDC_TIMER},
    {.channel = OUTPUT_LEDC_CHANNEL_1,
     .duty = 0,
     .gpio_num = OUTPUT_GPIO_1,
     .speed_mode = LEDC_MODE,
     .hpoint = 0,
     .timer_sel = LEDC_TIMER},
};

esp_err_t app_light_set_power(bool state)
{
    g_power = state;
    if (g_power)
    {
        light_set_cct_brightness(g_cct, g_brightness);
    }
    else
    {
        light_set_cct_brightness(g_cct, 0);
    }
    return ESP_OK;
}

esp_err_t app_light_set_cct(uint16_t cct)
{
    g_cct = cct;
    light_set_cct_brightness(g_cct, g_brightness);
    return ESP_OK;
}

esp_err_t app_light_set_brightness(uint16_t brightness)
{
    g_brightness = brightness;
    light_set_cct_brightness(g_cct, g_brightness);
    return ESP_OK;
}

void set_ledc_channel(uint8_t channel, uint32_t duty, uint16_t fade_time)
{
    ledc_set_fade_with_time(ledc_channel[channel].speed_mode,
                            ledc_channel[channel].channel, duty, fade_time);
    ledc_fade_start(ledc_channel[channel].speed_mode,
                    ledc_channel[channel].channel, LEDC_FADE_NO_WAIT);
}

esp_err_t light_set_cct_brightness(uint16_t cct, uint16_t brightness)
{
    uint16_t fade_time = 500;
    uint32_t total_duty = LEDC_LEVELS * brightness / 100;
    uint32_t cct_part = total_duty * (DEFAULT_MAX_CCT - cct) / (DEFAULT_MAX_CCT - DEFAULT_MIN_CCT);
    set_ledc_channel(WARM_OUTPUT, cct_part, fade_time);
    set_ledc_channel(COLD_OUTPUT, total_duty - cct_part, fade_time);
    vTaskDelay(fade_time / portTICK_PERIOD_MS);
    return ESP_OK;
}

void app_driver_init()
{
    app_reset_button_register(app_reset_button_create(BUTTON_GPIO_0, BUTTON_ACTIVE_LEVEL),
                              WIFI_RESET_BUTTON_TIMEOUT, FACTORY_RESET_BUTTON_TIMEOUT);

    // Init outputs
    ledc_timer_config(&ledc_timer);
    for (int ch = 0; ch < OUTPUT_NUM; ch++)
    {
        ledc_channel_config(&ledc_channel[ch]);
    }
    ledc_fade_func_install(0);
}