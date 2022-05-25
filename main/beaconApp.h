/**
 * @file beaconApp.h
 * @author Flynn Harrison
 * @brief 
 * @version 1.1
 * @date 2021-08-23
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef BEACONAPP_H
#define BEACONAPP_H

#include <stdint.h>

//// Select wether the device is a reciver or a beacon              // Remove
//#define DEVICE_BEACON   0             
//#define DEVICE_RECIVER  1
//#define BEACON_DEVICE_MODE DEVICE_RECIVER

#define BEACON_TX_PERIOD_SECONDS 0.9     // Transmit period

/**
 * @brief Broadcasts UUID over BLE every BEACON_TX_PERIOD
 * 
 */
void vBeaconTXTask(void *pvParameters);

/**
 * @brief Listens for broadcasts from BLE beacons
 * 
 * @param pvParameters 
 */
void vBeaconRXTask(void *pvParameters);

#endif