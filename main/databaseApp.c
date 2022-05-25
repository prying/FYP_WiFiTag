/**
 * @file databaseApp.c
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2021-9-22
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "databaseApp.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include "beaconBLE.h"
#include "http.h"
#include "globalQueues.h"

#define CYCLE_RATE_MS 1000*10

#define HTTP_SERVER				"159.196.72.33"
#define HTTP_PORT         "5000"
#define HTTP_DATABASE			"rssi_submit"
#define HTTP_VAR_PACKET_GROUP   "pkGroup"
#define HTTP_VAR_UUID           "uuid"
#define HTTP_VAR_RSSI           "rssi"
#define HTTP_VAR_TX_POWER       "txPower"
#define HTTP_VAR_DEVICEID		"deviceID"
#define HTTP_VAR_BUFF_SIZE		80

static const char TAG[] = "Database app";

void vDatabaseContact(void *pvParameters)
{
	char paramBuff[HTTP_VAR_BUFF_SIZE];
	int n;
	ble_beacon_recived_t *rd;
	TickType_t xLastWakeTick;

	ESP_LOGI(TAG, "vDatabaseContact app started");
	xLastWakeTick = xTaskGetTickCount();

	for(;;){
		vTaskDelayUntil(&xLastWakeTick, pdMS_TO_TICKS(CYCLE_RATE_MS));
		ESP_LOGI(TAG, "starting for next loop");
		

		// Check if queue exits yet
		if (beaconQueueHandle == NULL){
			ESP_LOGI(TAG, "Queue empty -> skipping");
			//continue;
		}

		// Loop que data
		while (uxQueueMessagesWaiting(beaconQueueHandle) > 0){

			// Get data off queue
			if (xQueueReceive(beaconQueueHandle, &rd, 0) != pdPASS){
				ESP_LOGE(TAG, "Failed to pull data from queue");
				vPortFree(rd);
				//continue;
			}

			// Construct http request
			// Non ideal code																																\|/ for testing we only care about the last number
			n = snprintf(paramBuff, HTTP_VAR_BUFF_SIZE, "%s?%s=%d&%s=%d&%s=%d", HTTP_DATABASE, HTTP_VAR_PACKET_GROUP,rd->packetGroup, HTTP_VAR_UUID, rd->uuid_32b[3], HTTP_VAR_RSSI, rd->rssi);
			if (n < 0 || n > HTTP_VAR_BUFF_SIZE){
				ESP_LOGE(TAG, "Unable to construct HTTP request paramters. Too long?");
				vPortFree(rd);
				//continue;
			}

			// Freeing data from queue, look at BLE GAP callback for malloc
			vPortFree(rd);

			// Send over HTTP
			if (http_send_request(HTTP_SERVER, HTTP_PORT, paramBuff) != HTTP_ERROR){
				ESP_LOGI(TAG, "Added entery to database");
			} else {
				ESP_LOGE(TAG, "Failed to add entery to database");
			}
			vTaskDelay(100/portTICK_PERIOD_MS);
		}

		//ESP_LOGI(TAG, "%s Stack high water mark: %u", __func__, uxTaskGetStackHighWaterMark(NULL));
	}

	vTaskDelete(NULL);
}

void databaseContact()
{
	char paramBuff[HTTP_VAR_BUFF_SIZE];
	int n;
	ble_beacon_recived_t *rd;

	// Check if queue exits yet
	if (beaconQueueHandle == NULL){
		ESP_LOGI(TAG, "Queue empty -> skipping");
		//continue;
	}

	// Loop que data
	while (uxQueueMessagesWaiting(beaconQueueHandle) > 0){

		// Get data off queue
		if (xQueueReceive(beaconQueueHandle, &rd, 0) != pdPASS){
			ESP_LOGE(TAG, "Failed to pull data from queue");
			vPortFree(rd);
			//continue;
		}

		// Construct http request
		// Non ideal code																																\|/ for testing we only care about the last number
		n = snprintf(paramBuff, HTTP_VAR_BUFF_SIZE, "%s?%s=%d&%s=%d&%s=%d&%s=%d", HTTP_DATABASE, HTTP_VAR_PACKET_GROUP,rd->packetGroup, HTTP_VAR_UUID, rd->uuid_32b[3], HTTP_VAR_RSSI, rd->rssi, HTTP_VAR_DEVICEID, rd->deviceID);
		if (n < 0 || n > HTTP_VAR_BUFF_SIZE){
			ESP_LOGE(TAG, "Unable to construct HTTP request paramters. Too long?");
			vPortFree(rd);
			//continue;
		}

		// Freeing data from queue, look at BLE GAP callback for malloc
		vPortFree(rd);

		// Send over HTTP
		if (http_send_request(HTTP_SERVER, HTTP_PORT, paramBuff) != HTTP_ERROR){
			ESP_LOGD(TAG, "Added entery to database");
		} else {
			ESP_LOGE(TAG, "Failed to add entery to database");
		}
		vTaskDelay(100/portTICK_PERIOD_MS);
	}

}