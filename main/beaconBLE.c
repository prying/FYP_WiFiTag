/**
 * @file beaconBLE.c
 * @author Flynn Harriosn
 * @brief 
 * @version 0.1
 * @date 2021-28-01
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "beaconBLE.h"

#include <esp_log.h>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_gap_ble_api.h>
#include <esp_bt_defs.h>

#include <string.h>

static const char TAG[] = "beacon BLE";

bool ble_is_beacon(uint8_t *buf)
{
	bool isBeacon = false;
	uint8_t *value;
	uint8_t len;
	uint8_t MSD[] = ADV_DATA_MAN_DATA;

	// Check manufacturer specific data
	value = esp_ble_resolve_adv_data(buf, 0xFF, &len);
	if (0 == memcmp(value, MSD, len) && len == 4)
	{
		isBeacon = true;
	}

	return isBeacon;
}

esp_err_t ble_beacon_decode(uint8_t *buf, ble_beacon_recived_t* received_data)
{
	esp_err_t ret = ESP_OK;
	uint8_t *value;
	uint8_t len;
	
	// Get MSD
	value = esp_ble_resolve_adv_data(buf, 0xFF, &len);
	if(len == ADV_DATA_MAN_LEN)
	{
		memcpy(received_data->msd, value, len);
	}
	else
	{
		ESP_LOGE(TAG, "%s BLE recived decode of MSD failed\n", __func__);
		return ESP_FAIL;
	}

	// Get 32b UUID
	value = esp_ble_resolve_adv_data(buf, 0x16, &len);
	if(len == ADV_DATA_SERVICE_LEN)
	{
		memcpy(received_data->uuid_32b, value, len);
	}
	else
	{
		ESP_LOGE(TAG, "%s BLE recived decode of UUID32b failed\n", __func__);
		return ESP_FAIL;
	}

	// Get Tx power
	value = esp_ble_resolve_adv_data(buf, 0x0A, &len);
	if(len == 1)
	{
		//memcpy(received_data->TxPower, value, len);
		received_data->TxPower = *value;
	}
	else
	{
		ESP_LOGE(TAG, "%s BLE recived decode of TxPower failed\n", __func__);
		return ESP_FAIL;
	}

	return ret;
}

esp_err_t ble_beacon_appRegister(esp_gap_ble_cb_t callback)
{
	esp_err_t ret;

	ret = esp_ble_gap_register_callback(callback);
	if (ret)
	{
		ESP_LOGE(TAG, "%s BLE GAP callback registration failed, error: %s\n", __func__, esp_err_to_name(ret));
	}
	
	return ret;
}

esp_err_t ble_init(void)
{
	esp_err_t ret;
	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

	// Setup BT controller
	ret = esp_bt_controller_init(&bt_cfg);
	if (ret)
	{
		ESP_LOGE(TAG, "%s controller init failed, error: %s\n", __func__, esp_err_to_name(ret));
		return ret;
	}

	// TODO add sleep modes where -> this might need to be called evey loop
	ret = esp_bluedroid_init();
	if (ret)
	{
		ESP_LOGE(TAG, "%s bluedroid init failed, error: %s\n", __func__, esp_err_to_name(ret));
		return ret;
	}

	// Setup succesfull
	ESP_LOGI(TAG, "%s BLE initialized sucsessfully\n", __func__);
	return ESP_OK;
}

esp_err_t ble_deinit(void)
{
	esp_err_t ret;

	ret = esp_bluedroid_deinit();
	if (ret)
	{
		ESP_LOGE(TAG, "%s bluedroid deinit failed, error: %s\n", __func__, esp_err_to_name(ret));
		return ret;
	}

	ret = esp_bt_controller_deinit();
	if (ret)
	{
		ESP_LOGE(TAG, "%s controller deinit failed, error: %s\n", __func__, esp_err_to_name(ret));
		return ret;
	}

	ESP_LOGI(TAG,"%s BLE deinitialised successfully\n", __func__);
	return ESP_OK;
}

esp_err_t ble_enable(void)
{
	esp_err_t ret;

	ret = esp_bt_controller_enable(BEACONBLE_BT_MODE);
	if (ret)
	{
		ESP_LOGE(TAG, "%s controller enable failed, error: %s\n", __func__, esp_err_to_name(ret));
		return ret;
	}

	ret = esp_bluedroid_enable();
	if (ret)
	{
		ESP_LOGE(TAG, "%s bluedroid enable failed, error: %s\n", __func__, esp_err_to_name(ret));
		return ret;
	}

	// Setup succesfull
	ESP_LOGI(TAG, "%s BLE enabled sucsessfully\n", __func__);
	return ESP_OK;
}

esp_err_t ble_disable(void)
{
	esp_err_t ret;

	ret = esp_bluedroid_disable();
	if (ret)
	{
		ESP_LOGE(TAG, "%s bluedroid disable failed, error: %s\n", __func__, esp_err_to_name(ret));
		return ret;
	}

	ret = esp_bt_controller_disable();
	if (ret)
	{
		ESP_LOGE(TAG, "%s controller disable failed, error: %s\n", __func__, esp_err_to_name(ret));
		return ret;
	}

	ESP_LOGI(TAG,"%s BLE disabled successfully\n", __func__);
	return ESP_OK;
}

 esp_err_t ble_start()
{
	esp_err_t ret;
	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

	// Setup BT controller
	ret = esp_bt_controller_init(&bt_cfg);
	if (ret)
	{
		ESP_LOGE(TAG, "%s controller init failed, error: %s\n", __func__, esp_err_to_name(ret));
		return ret;
	}

	ret = esp_bt_controller_enable(BEACONBLE_BT_MODE);
	if (ret)
	{
		ESP_LOGE(TAG, "%s controller enable failed, error: %s\n", __func__, esp_err_to_name(ret));
		return ret;
	}

	// TODO add sleep modes where -> this might need to be called evey loop
	ret = esp_bluedroid_init();
	if (ret)
	{
		ESP_LOGE(TAG, "%s bluedroid init failed, error: %s\n", __func__, esp_err_to_name(ret));
		return ret;
	}

	ret = esp_bluedroid_enable();
	if (ret)
	{
		ESP_LOGE(TAG, "%s bluedroid enable failed, error: %s\n", __func__, esp_err_to_name(ret));
		return ret;
	}

	// Setup succesfull
	ESP_LOGI(TAG, "%s BLE initialized & enabled sucsessfully\n", __func__);
	return ESP_OK;
}

 esp_err_t ble_stop()
{
	ble_disable();
	ble_deinit();

	return ESP_OK;
}