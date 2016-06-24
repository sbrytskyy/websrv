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

#define MAX_PACKET_SIZE 1024

#define CONFIG_HTTP_DEFAULT_SSL_OPTIONS     SSL_DISPLAY_CERTS

struct connstruct
{
    struct connstruct *next;
    int handle;
    SSL *ssl;
    uint8_t is_ssl;
};

struct serverstruct
{
    struct serverstruct *next;
    int handle;
    int is_ssl;
//    SSL_CTX *ssl_ctx;
};

int init_server_socket(uint16_t port);
int process_incoming_connections(int server_socket, int secured_server_socket);
int set_socket_write_mode(int client_socket);

#endif /* SERVER_H_ */
