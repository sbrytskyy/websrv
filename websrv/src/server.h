/*
 * server.h
 *
 *  Created on: Apr 28, 2016
 *      Author: sbrytskyy
 */

#ifndef SERVER_H_
#define SERVER_H_

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#include "structures.h"

#include "worker.h"

#define EPOLL_ARRAY_SIZE   64

#define MAX_PACKET_SIZE 512

int init_server_socket(uint16_t port);
int process_incoming_connections(int server_socket);

#endif /* SERVER_H_ */
