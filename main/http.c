/**
 * @file http.c
 * @author Flynn Harrison
 * @brief 
 * @version 0.1
 * @date 2021-9-20
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "http.h"

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#define TXBUFF_SIZE 100

#define HTTP_PORT "80"

static const char TAG[] = "HTTP api";

int http_send_request(const char* url, const char* port, const char* path)
{
	struct addrinfo *res;
	int err, s, n;
	char txBuff[TXBUFF_SIZE];

	// Configure socket type
	const struct addrinfo hints = {
		.ai_family = AF_INET,			// IPv4
		.ai_socktype = SOCK_STREAM,		// TCP - might change to udp later
	};

	// Construct and send http header
	// Needs to be optimised later on! Does it block? ¯\_(ツ)_/¯
	n = snprintf(txBuff, TXBUFF_SIZE, "POST /%s HTTP/1.0\r\nHost: %s:%s\r\n\r\n", path, url, port);
	if (n < 0 || n >= TXBUFF_SIZE){
		ESP_LOGE(TAG, "Get request construction failed, n: %d", n);
		return HTTP_ERROR;
	}

	// DNS lookup
	// If it fails then there might not be a internet connection
	err = getaddrinfo(url, port, &hints, &res);
	if (err != 0){
		ESP_LOGE(TAG, "DNS lookup failed");
		return HTTP_ERROR;
	}

	// Allocate socket
	s = socket(res->ai_family, res->ai_socktype, 0);
	if (s < 0){
		ESP_LOGE(TAG, "Failed to create socket");
		close(s);
		freeaddrinfo(res);
		return HTTP_ERROR;
	}

	// Connect to server
	if (connect(s, res->ai_addr, res->ai_addrlen) != 0){
		ESP_LOGE(TAG, "Failed to connect to server %s", url);
		close(s);
		freeaddrinfo(res);
		return HTTP_ERROR;
	}

	freeaddrinfo(res);

	if (write(s, txBuff, n) < 0 ){
		ESP_LOGE(TAG, "Failed to write to socket");
		close(s);
		return HTTP_ERROR;
	}

	ESP_LOGD(TAG, "HTTP request sent");

	// Currently dont care about http response
	vTaskDelay(50 / portTICK_PERIOD_MS);
	close(s);

	return 1;
}

