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

	if (listen (server_socket, 10) < 0)
	{
		perror ("Error listening server socket.");
		exit (EXIT_FAILURE);
	}

	printf("Listening to server socket %d.\n", DEFAULT_SERVER_PORT);

	process_incoming_connections(server_socket);

	close(server_socket);

	return (EXIT_SUCCESS);
}

int main(void)
{
	return runServer();
}
