/**
 * @file WiFi.c
 * @author Flynn Harrison
 * @brief connects to wifi following https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/wifi.html#wi-fi-start-phase
 * @version 0.1
 * @date 2021-9-20
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "WiFi.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "string.h"

#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "sdkconfig.h"

#include "lwip/sys.h"
#include "lwip/err.h"

// Setting set by config
#define WIFI_MAX_RETRY 10000     //CONFIG_ESP_MAXIMUM_RETRY
#define WIFI_RECONNECT_DELAY 5000

#define LISTEN_INTERVAL 3
//#define PS_MODE WIFI_PS_MIN_MODEM
#define PS_MODE WIFI_PS_MAX_MODEM

#if defined(CONFIG_EAP_USE_CERTIFICATE) 
extern uint8_t ca_pem_start[] asm("_binary_ca_pem_start");
extern uint8_t ca_pem_end[]   asm("_binary_ca_pem_end");
#endif

// Event group for keeping track of connection status 
// Bit 0 - is wifi connected
// bit 1 - connection failed after max attempt 
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT  0x0001
#define WIFI_FAILED_BIT     0x0002

// Info logging
static const char *TAG = "WiFi station";

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    static int s_retry_num = 0;
    
    ESP_LOGI(TAG, "Event handler called with base=%s, event_id=%d", event_base, event_id);

    // Attempt to connect wifi station to AP
    if (event_base == WIFI_EVENT)
    {
        if (event_id == WIFI_EVENT_STA_START)
        {
            esp_wifi_connect();
        }
        else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAILED_BIT);
            xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            vTaskDelay(pdMS_TO_TICKS(WIFI_RECONNECT_DELAY));
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retrying to connect to wifi attempt %d.", s_retry_num);
        }
    }

    // Check if it has connected and obtained an IP
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        xEventGroupClearBits(s_wifi_event_group, WIFI_FAILED_BIT);
        ESP_LOGI(TAG, "Connected with ip: %d.%d.%d.%d", IP2STR(&event->ip_info.ip));
    }
}

void WiFiManageTask(void * pvParam)
{
#if defined(CONFIG_EAP_USE_CERTIFICATE)
    unsigned int ca_pem_bytes = ca_pem_end - ca_pem_start;
#endif

    // WiFi/LwIP init phase
    // Init wifi event group
    s_wifi_event_group = xEventGroupCreate();

    // LwIP through netif init
    ESP_ERROR_CHECK(esp_netif_init());

    // Init application event callback and register event handler
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));

    // Create network interface binding station with TCP/IP stack 
    esp_netif_create_default_wifi_sta();

    // Create wifi driver task and initialise the wifi driver
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    // WiFi config phase
    wifi_config_t wifi_cfg = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .listen_interval = LISTEN_INTERVAL,
#if defined(CONFIG_WPA_WPA2_PERSONAL)
            .password = CONFIG_WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
#endif

        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));

#if defined(CONFIG_WPA_WPA2_ENTERPRISE)
    ESP_ERROR_CHECK(esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)CONFIG_EAP_ID, strlen(CONFIG_EAP_ID)));

#if defined(CONFIG_EAP_USE_CERTIFICATE)
    ESP_ERROR_CHECK( esp_wifi_sta_wpa2_ent_set_ca_cert(ca_pem_start, ca_pem_bytes));
#endif

    ESP_ERROR_CHECK(esp_wifi_sta_wpa2_ent_set_username((uint8_t *)CONFIG_EAP_USERNAME, strlen(CONFIG_EAP_USERNAME)));
    ESP_ERROR_CHECK(esp_wifi_sta_wpa2_ent_set_password((uint8_t *)CONFIG_EAP_PASSWORD, strlen(CONFIG_EAP_PASSWORD)));

    ESP_ERROR_CHECK(esp_wifi_sta_wpa2_ent_enable());
#endif

    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_set_ps(PS_MODE);
    ESP_LOGI(TAG, "wifi initilisation finished.");

    ESP_LOGI(TAG, "HIGH TIDE %d", uxTaskGetStackHighWaterMark(NULL));
    // Connection and 'Got IP' phase
    for ( ;; )
    {
        // Wait for connection (or fail) from the event handler
        xEventGroupWaitBits(s_wifi_event_group,
            WIFI_FAILED_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);  

        vTaskDelay(pdMS_TO_TICKS(5000));
    }


    // free event handler and event group
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
    vTaskDelete(NULL);
}

void WiFiWaitUntillConnected()
{
    xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);
}