#include <sdkconfig.h>
#include <string.h>
#include <esp_log.h>

#include <app_reset.h>
#include "app_priv.h"

#define RMT_TX_CHANNEL RMT_CHANNEL_0
/* This is the button that is used for toggling the power */
#define BUTTON_GPIO 4
#define BUTTON_ACTIVE_LEVEL 0
/* This is the GPIO on which the power will be set */

#define OUTPUT_GPIO_1 23ULL
#define OUTPUT_GPIO_2 22ULL

#define WIFI_RESET_BUTTON_TIMEOUT 3
#define FACTORY_RESET_BUTTON_TIMEOUT 10

esp_err_t app_driver_set_gpio(const char *name, bool state)
{
    if (strcmp(name, "Warm") == 0)
    {
        gpio_set_level(OUTPUT_GPIO_1, state);
    }
    else if (strcmp(name, "Cold") == 0)
    {
        gpio_set_level(OUTPUT_GPIO_2, state);
    }
    else
    {
        return ESP_FAIL;
    }
    return ESP_OK;
}

void app_driver_init()
{
    app_reset_button_register(app_reset_button_create(BUTTON_GPIO, BUTTON_ACTIVE_LEVEL),
                              WIFI_RESET_BUTTON_TIMEOUT, FACTORY_RESET_BUTTON_TIMEOUT);

    /* Configure power */
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 1,
    };
    uint64_t pin_mask = (((uint64_t)1 << OUTPUT_GPIO_1) | ((uint64_t)1 << OUTPUT_GPIO_2));
    io_conf.pin_bit_mask = pin_mask;
    /* Configure the GPIO */
    gpio_config(&io_conf);
    gpio_set_level(OUTPUT_GPIO_1, false);
    gpio_set_level(OUTPUT_GPIO_2, false);
}
