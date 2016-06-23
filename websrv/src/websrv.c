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
#include <pthread.h>

#include "server.h"
#include "storage.h"
#include "worker.h"
#include "test.h"
#include "utils.h"

#define DEFAULT_SERVER_PORT 8080
#define DEFAULT_SECURED_SERVER_PORT 8443

int runServer()
{
	int server_sockets[2];

	server_sockets[0] = init_server_socket(DEFAULT_SERVER_PORT);
	if (server_sockets[0] < 0)
	{
		return EXIT_FAILURE;
	}

	server_sockets[1] = init_server_socket(DEFAULT_SECURED_SERVER_PORT);
	if (server_sockets[1] < 0)
	{
		return EXIT_FAILURE;
	}

	dprint("Server socket initialized successfully.\n");

	int result = init_context_storage();
	if (result == -1)
	{
		perror("Error creating contex storage.");
		return -1;
	}
	result = start_workers();
	if (result == -1)
	{
		perror("Error creating worker.");
		return -1;
	}

	process_incoming_connections(server_sockets);

	stop_workers();
	destroy_context_storage();
	pthread_exit(EXIT_SUCCESS);
}

int main(void)
{
#ifdef DEBUG
	setvbuf(stdout, NULL, _IONBF, 0);
#endif
//	return test_storage();
	return runServer();
}
