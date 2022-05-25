/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "beaconApp.h"
#include "beaconBLE.h"
#include "WiFi.h"
#include "databaseApp.h"

#include "globalQueues.h"

#define BEACON_QUEUE_LEN 10
QueueHandle_t beaconQueueHandle = NULL;	// evil global

static const char TAG[] = "Main";

void app_main(void)
{
	esp_err_t ret;

	// Init NVS
	ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ESP_ERROR_CHECK(nvs_flash_init());
	}

	ESP_LOGI(TAG, "Device ready");

	// Create Queue for found beacons 
	beaconQueueHandle = xQueueCreate(BEACON_QUEUE_LEN, sizeof(ble_beacon_recived_t));
	if (beaconQueueHandle == NULL){
		ESP_LOGE(TAG, "Queue failed to be created");
		return;
	}

	// If WiFi enabled
	xTaskCreate(
		WiFiManageTask,
		"WiFi manage",
		8000,
		NULL,
		3,
		NULL
	);

	xTaskCreate(
		vBeaconRXTask,      // Task function
		"BLE Beacon",       // Name
		8000,           	// Stack size (bytes)
		NULL,               // Parameters passed to the function
		2,                  // Priority
		NULL);              // Task handle
}
