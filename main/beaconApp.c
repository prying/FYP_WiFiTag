/**
 * @file beaconApp.c
 * @author Flynn Harrison
 * @brief
 * @version 1.1
 * @date 2021-08-23
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "beaconApp.h"

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_defs.h"
#include "esp_log.h"

#include "beaconBLE.h"
#include "globalQueues.h"
#include "databaseApp.h"
#include "WiFi.h"

// Frequency between adverise pulses
#define CYCLE_RATE_MS_RX 1000*8 // How frequently the RX app runs
#define RX_RECIVE_TIME	1000*2  // How long it waits for a response

#define DEVICEID 1				// Reciver device ID ------- will be subject to change in format

#define BEACON_LIST_SIZE 20

// ESP_LOGx tag
static const char TAG[] = "beacon module";

struct list_s {
  uint32_t list[BEACON_LIST_SIZE];
  size_t  size;
};

static esp_ble_scan_params_t ble_scan_params = {
    .scan_type              = SCN_PARAM_SCAN_TYPE,
    .own_addr_type          = SCN_PARAM_OWM_ADDR_TYPE,
    .scan_filter_policy     = SCN_PARAM_SCAN_FILTER_POLICY,
    .scan_interval          = SCN_PARAM_SCAN_INTERVAL,
    .scan_window            = SCN_PARAM_SCAN_WINDOW,
    .scan_duplicate         = SCN_PARAM_SCAN_DUPLICATE
};

static uint8_t isInList(struct list_s * beaconList, uint8_t id);
static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

void vBeaconRXTask(void *pvParameters)
{
	TickType_t xLastWakeTick;
	esp_err_t ret;
	//uint32_t scan_duration = 3;

	ret = ble_start();
	if (ret){
		vTaskDelete(NULL);
	}

	ret = ble_beacon_appRegister(esp_gap_cb);
	if (ret){
		vTaskDelete(NULL);
	}

	// RX logic
	ret = esp_ble_gap_set_scan_params(&ble_scan_params);
	if (ret){
		ESP_LOGE(TAG, "%s Failed to configure scan params, error: %s", __func__, esp_err_to_name(ret));
		vTaskDelete(NULL);
	}


	ESP_LOGI(TAG, "%s Started RX application\n", __func__);
	xLastWakeTick = xTaskGetTickCount();
	for(;;){
		// Wait for next cycle to start before unblocking
		vTaskDelayUntil(&xLastWakeTick, pdMS_TO_TICKS(CYCLE_RATE_MS_RX));

		// Wait till Wifi is connected or wait for reconnection
		WiFiWaitUntillConnected();

		ESP_LOGD(TAG, "Starting scan");
		esp_ble_gap_start_scanning(0);
		vTaskDelay(pdMS_TO_TICKS(RX_RECIVE_TIME));
		esp_ble_gap_stop_scanning();
		ESP_LOGD(TAG, "Finish Scan");

		// Send to database
		databaseContact();
	}

	vTaskDelete(NULL);
}

/**
 * @brief Callback for GAP events
 *
 * @param event
 * @param param
 */
static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
	static unsigned int packetGroup = 0;		// Keeps track of what beacons were recived at the same time 
  static struct list_s heardBeacons;
	esp_err_t ret;

	switch (event)
	{

	case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:{
		break;
	}

	// Indicate scan start operation success status
	case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
		ret = param->scan_start_cmpl.status;
		if (ret != ESP_BT_STATUS_SUCCESS){
			ESP_LOGE(TAG, "%s Scan failed to start, error: %s", __func__, esp_err_to_name(ret));
		} else {
			packetGroup++;
      heardBeacons = (const struct list_s){ 0 };
			ESP_LOGD(TAG, "%s Started scan successfull", __func__);
		}
		break;

	// When one scan result ready, the event comes each time
	case ESP_GAP_BLE_SCAN_RESULT_EVT:{
		esp_ble_gap_cb_param_t *scan_result = param;
		ESP_LOGD(TAG, "%s Scan results event", __func__);
    
    if (scan_result->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT)
    {
      if(ble_is_beacon(scan_result->scan_rst.ble_adv))
      {
				// Allocate data structure memory 		REMEMBER TO FREE!!!
				ble_beacon_recived_t *received_data;
				received_data = pvPortMalloc(sizeof(ble_beacon_recived_t));
				if (received_data == NULL)
        {
					ESP_LOGE(TAG, "Failed to allocate memory to received_data");
					break;
				}

				// Fillout data
				ble_beacon_decode(scan_result->scan_rst.ble_adv, received_data);
				received_data->rssi = scan_result->scan_rst.rssi;
				received_data->packetGroup = packetGroup;
				received_data->deviceID = DEVICEID;

        // Check if beacon has already been discovered in this scan
        if (isInList(&heardBeacons, received_data->uuid_32b[3]))
        {
          vPortFree(received_data);
          break;
        }
        else
        {
          heardBeacons.list[heardBeacons.size++] = received_data->uuid_32b[3];
        }

				// Add to queue 
				if(xQueueSend(beaconQueueHandle, &received_data, 0) != pdTRUE)
        {
					ESP_LOGE(TAG, "Failed to add memory to beaconQueueHandle queue");
					vPortFree(received_data);
					break;
				}

				// Share over serial
				ESP_LOGD(TAG, "~~Beacon Found~~\n");
				ESP_LOGD(TAG, "UUID_32b: %02x %02x %02x %02x\n",received_data->uuid_32b[0], received_data->uuid_32b[1], received_data->uuid_32b[2], received_data->uuid_32b[3]);
				ESP_LOGD(TAG, "TxPower: %d dBm\n",received_data->TxPower);
				ESP_LOGD(TAG, "RSSI: %d dBm\n",received_data->rssi);
			}
    }
		break;
	}

	case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
        if ((ret = param->scan_stop_cmpl.status) != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(TAG, "%s Scan stop failed: %s", __func__ , esp_err_to_name(ret));
        } else {
            ESP_LOGD(TAG, "%s Stoped scan successfully", __func__);
        }
        break;

	default:
		break;
	}
}

static uint8_t isInList(struct list_s * beaconList, uint8_t id)
{
  for (int i = 0; i < beaconList->size; i++)
  {
    if (id == beaconList->list[i])
    {
      return 1;
    }
  }

  return 0;
}