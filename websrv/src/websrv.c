/*
 ============================================================================
 Name        : websrv.c
 Author      : Serhiy Brytskyy
 Version     :
 Copyright   : Your copyright notice
 Description :
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>

#include "server.h"

#define DEFAULT_SERVER_PORT 8080

int runServer()
{
	int server_socket = init_server_socket(DEFAULT_SERVER_PORT);
	if (server_socket < 0)
	{
		return EXIT_FAILURE;
	}

	puts("Server socket initialized successfully.");

	process_incoming_connections(server_socket);

	return (EXIT_SUCCESS);
}

int main(void)
{
	return runServer();
}
