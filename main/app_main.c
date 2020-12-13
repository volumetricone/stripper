#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_event.h>
#include <nvs_flash.h>

#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_types.h>
#include <esp_rmaker_standard_params.h>
#include <esp_rmaker_standard_devices.h>
#include <esp_rmaker_ota.h>
#include <esp_rmaker_schedule.h>

#include <app_wifi.h>
#include "app_priv.h"

static const char *TAG = "app_main";

/* Callback to handle commands received from the RainMaker cloud */
static esp_err_t write_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param,
                          const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx)
{
    if (ctx)
    {
        ESP_LOGI(TAG, "Received write request via : %s", esp_rmaker_device_cb_src_to_str(ctx->src));
    }
    const char *device_name = esp_rmaker_device_get_name(device);
    const char *param_name = esp_rmaker_param_get_name(param);

    if (strcmp(param_name, ESP_RMAKER_DEF_POWER_NAME) == 0)
    {
        ESP_LOGI(TAG, "Received value %s for %s %s", val.val.b ? "True" : "False", device_name, param_name);
        app_light_set_power(val.val.b);
    }
    else if (strcmp(param_name, ESP_RMAKER_DEF_CCT_NAME) == 0)
    {
        ESP_LOGI(TAG, "Received value %d for %s %s", val.val.i, device_name, param_name);
        app_light_set_cct(val.val.i);
    }
    else if (strcmp(param_name, ESP_RMAKER_DEF_BRIGHTNESS_NAME) == 0)
    {
        ESP_LOGI(TAG, "Received value %d for %s %s", val.val.i, device_name, param_name);
        app_light_set_brightness(val.val.i);
    }
    else
    {
        // Silently ignore unkown params
        return ESP_OK;
    }

    esp_rmaker_param_update_and_report(param, val);
    return ESP_OK;
}

/* Event handler for catching RainMaker events */
static void event_handler(void *arg, esp_event_base_t event_base,
                          int event_id, void *event_data)
{
    if (event_base == RMAKER_EVENT)
    {
        switch (event_id)
        {
        case RMAKER_EVENT_INIT_DONE:
            ESP_LOGI(TAG, "RainMaker Initialised.");
            break;
        case RMAKER_EVENT_CLAIM_STARTED:
            ESP_LOGI(TAG, "RainMaker Claim Started.");
            break;
        case RMAKER_EVENT_CLAIM_SUCCESSFUL:
            ESP_LOGI(TAG, "RainMaker Claim Successful.");
            break;
        case RMAKER_EVENT_CLAIM_FAILED:
            ESP_LOGI(TAG, "RainMaker Claim Failed.");
            break;
        case RMAKER_EVENT_REBOOT:
            ESP_LOGI(TAG, "Rebooting in %d seconds.", *((uint8_t *)event_data));
            break;
        case RMAKER_EVENT_WIFI_RESET:
            ESP_LOGI(TAG, "Wi-Fi credentials reset.");
            break;
        case RMAKER_EVENT_FACTORY_RESET:
            ESP_LOGI(TAG, "Node reset to factory defaults.");
            break;
        case RMAKER_EVENT_MQTT_CONNECTED:
            ESP_LOGI(TAG, "MQTT Connected.");
            break;
        case RMAKER_EVENT_MQTT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT Disconnected.");
            break;
        case RMAKER_EVENT_MQTT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT Published. Msg id: %d.", *((int *)event_data));
            break;
        default:
            ESP_LOGW(TAG, "Unhandled RainMaker Event: %d", event_id);
        }
    }
    else
    {
        ESP_LOGW(TAG, "Invalid event received!");
    }
}

void app_main()
{
    /* Initialize Application specific hardware drivers and
     * set initial state.
     */
    app_driver_init();

    /* Initialize NVS. */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    /* Initialize Wi-Fi. Note that, this should be called before esp_rmaker_init()
     */
    app_wifi_init();

    /* Register an event handler to catch RainMaker events */
    ESP_ERROR_CHECK(esp_event_handler_register(RMAKER_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));

    /* Initialize the ESP RainMaker Agent.
     * Note that this should be called after app_wifi_init() but before app_wifi_start()
     * */
    esp_rmaker_config_t rainmaker_cfg = {
        .enable_time_sync = false,
    };
    esp_rmaker_node_t *node = esp_rmaker_node_init(&rainmaker_cfg, "Dual LED strip controller", "VOLED2CH1");
    if (!node)
    {
        ESP_LOGE(TAG, "Could not initialise node. Aborting!!!");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        abort();
    }

    /* Create a device and add the relevant parameters to it */
    esp_rmaker_device_t *led_strip_device = esp_rmaker_lightbulb_device_create("LED Strip", NULL, NULL);
    esp_rmaker_device_add_cb(led_strip_device, write_cb, NULL);

    esp_rmaker_param_t *brightness = esp_rmaker_brightness_param_create(ESP_RMAKER_DEF_BRIGHTNESS_NAME, DEFAULT_BRIGHTNESS);
    esp_rmaker_device_add_param(led_strip_device, brightness);

    esp_rmaker_param_t *color_temperature = esp_rmaker_cct_param_create(ESP_RMAKER_DEF_CCT_NAME, DEFAULT_CCT);
    esp_rmaker_device_add_param(led_strip_device, color_temperature);

    esp_rmaker_node_add_device(node, led_strip_device);

    /* Enable OTA */
    esp_rmaker_ota_config_t ota_config = {
        .server_cert = ESP_RMAKER_OTA_DEFAULT_SERVER_CERT,
    };
    esp_rmaker_ota_enable(&ota_config, OTA_USING_PARAMS);

    /* Enable scheduler */
    esp_err_t esp_rmaker_schedule_enable(void);

    /* Start the ESP RainMaker Agent */
    esp_rmaker_start();

    /* Start the Wi-Fi.
     * If the node is provisioned, it will start connection attempts,
     * else, it will start Wi-Fi provisioning. The function will return
     * after a connection has been successfully established
     */
    err = app_wifi_start(POP_TYPE_RANDOM);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not start Wifi. Aborting!!!");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        abort();
    }
}
