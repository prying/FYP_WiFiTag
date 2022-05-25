/**
 * @file WiFi.h
 * @author Flynn Harrison
 * @brief Simple connection to WiFi
 * @version 0.1
 * @date 2021-9-20
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef WIFI_H
#define WIFI_H

#include "esp_err.h"

/**
 * @brief Connects to acsess point specified by CONFIG_ESP_WIFI_SSID and CONFIG_ESP_WIFI_PASSWORD in esp-idf configuration
 * 
 * @return esp_err_t 
 */
void WiFiManageTask(void * pvParam);

void WiFiWaitUntillConnected();

#endif