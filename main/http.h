/**
 * @file http.h
 * @author Flynn Harrison
 * @brief 
 * @version 0.1
 * @date 2021-9-22
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef HTTP_H
#define HTTP_H

#define HTTP_ERROR -1

int http_send_request(const char* url, const char* port, const char* path);

#endif