/*
 * server.h
 *
 *  Created on: Apr 28, 2016
 *      Author: sbrytskyy
 */

#ifndef SERVER_H_
#define SERVER_H_

#include <stdint.h>

#include "ssl.h"

#define MAX_PACKET_SIZE	8800  /* FF3=4096, IE7=8760 */

#define CONFIG_HTTP_DEFAULT_SSL_OPTIONS     SSL_DISPLAY_CERTS

struct connection_info
{
    struct connection_info *next;
    int handle;
    SSL *ssl;
    uint8_t is_ssl;
};

int init_server_socket(uint16_t port);
int process_incoming_connections(int server_socket, int secured_server_socket);
int set_socket_write_mode(int client_socket);

#endif /* SERVER_H_ */
