/**
 * @file beaconBLE.h
 * @author Flynn Harrison
 * @brief
 * @version 0.1
 * @date 2021-08-28
 *
 * @copyright Copyright (c) 2021
 *
 */

#ifndef BEACONBLE_H
#define BEACONBLE_H

#include <esp_gap_ble_api.h>

#define BEACONBLE_BT_MODE ESP_BT_MODE_BLE
// TODO: https://docs.espressif.com/projects/esp-idf/en/v3.0-rc1/api-reference/system/sleep_modes.html#wifi-bt-and-sleep-modes
// BLE brodcast setting and data (for an idea on how its structured look at https://jimmywongiot.com/2019/08/13/advertising-payload-format-on-ble/)
#define ADV_DATA_SCAN_RSP 		false														//
#define ADV_DATA_NAME 			false														//
#define ADV_DATA_TXPOWER 		true  														// Send TX power
#define ADV_DATA_MIN_INT 		0xFFFF														//
#define ADV_DATA_MAX_INT 		0xFFFF														//
#define ADV_DATA_APPEARANCE 	0x0200     													// Generic Tag
#define ADV_DATA_MAN_LEN 		4 															// Manufacture data length
#define ADV_DATA_SERVICE_LEN 	4 															// Serivde is meant to hold 16bit UUID (need to check if its a 'used' one)
#define ADV_DATA_UUID_LEN 		0   														// UUID length UUID_128b DISABLED
#define ADV_DATA_FLAG 			(ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT) // Generic discovery + BR/EDR not suported

#define ADV_DATA_MAN_DATA    { 0xFF, 0xFF, 'F', 'H'}
#define ADV_DATA_UUID_128b   { 0x33, 0xb4, 0x88, 0x71, 0x3c, 0xbd, 0x47, 0xa1, 0xaa, 0xa4, 0x74, 0x4f, 0xa3, 0xd8, 0xd4, 0x63 } 
#define ADV_DATA_UUID_32b    { 'F', 'Y', 'P', 'A'}

#define SCN_PARAM_SCAN_TYPE 			BLE_SCAN_TYPE_PASSIVE		// Change do passive later
#define SCN_PARAM_OWM_ADDR_TYPE 		BLE_ADDR_TYPE_PUBLIC
#define SCN_PARAM_SCAN_FILTER_POLICY 	BLE_SCAN_FILTER_ALLOW_ALL // Look into whitelist https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/bluetooth/esp_gap_ble.html#_CPPv421esp_ble_scan_filter_t
#define SCN_PARAM_SCAN_INTERVAL 		0x50     //0xFA0						// Time interval since last scan start = N * 0.625 msec
#define SCN_PARAM_SCAN_WINDOW 			0x30     //0xFA0						// Scan time = N * 0.625 msec
#define SCN_PARAM_SCAN_DUPLICATE 		BLE_SCAN_DUPLICATE_ENABLE 


typedef struct{
	uint8_t msd[ADV_DATA_MAN_LEN];
	uint8_t uuid_32b[ADV_DATA_SERVICE_LEN];
	uint8_t TxPower;
	int8_t rssi;
	int packetGroup;							// Needs to be removed for non testing as this can only recive so many packets (This value will itterate once per scan cycle)
	int8_t deviceID;
}ble_beacon_recived_t;

/**
 * @brief Checks if the recived data is a beacon. This is determined through msd = ADV_DATA_MAN_DATA
 * 
 * @param buf 
 * @return true 
 * @return false 
 */
bool ble_is_beacon(uint8_t *buf);

/**
 * @brief Decoads advertised data
 *
 * @param buf (scan_rst.ble_adv)
 * @param len (scan_rst.adv_data_len)
 * @param braodcast
 * @return esp_err_t
 */
esp_err_t ble_beacon_decode(uint8_t *buf, ble_beacon_recived_t* received_data);

/**
 * @brief Initilises bt controller and blueroid stack. (Following call should be ble_enable())
 *
 * @return esp_err_t
 */
esp_err_t ble_init(void);

/**
 * @brief Deinitilise blueroid and bt controller (Must call ble_disable() first!)
 *
 * @return esp_err_t
 */
esp_err_t ble_deinit(void);

/**
 * @brief Enable BLE. Call after ble_init() has been called
 *
 * @return esp_err_t
 */
esp_err_t ble_enable(void);

/**
 * @brief Disable BLE (but not deinitilise them)
 *
 * @return esp_err_t
 */
esp_err_t ble_disable(void);

/**
 * @brief initilise and enable bt controller and stack
 *
 * @return esp_err_t
 */
esp_err_t ble_start(void);

/**
 * @brief disables and deinitlises bt controller and stack
 *
 * @return esp_err_t
 */
esp_err_t ble_stop(void);

/**
 * @brief Register GAP callback function
 *
 * @return esp_err_t
 */
esp_err_t ble_beacon_appRegister(esp_gap_ble_cb_t callback);

#endif