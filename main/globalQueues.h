/**
 * @file globalQueues.h
 * @author Flynn Harrison
 * @brief 
 * @version 0.1
 * @date 2021-09-30
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "freertos/queue.h"

#ifndef GLOBALQUEUES_H
#define GLOBALQUEUES_H

/**
 * @brief Queue for detected beacons
 * -Yes it does feel dirty
 * 
 */
extern QueueHandle_t beaconQueueHandle;

#endif