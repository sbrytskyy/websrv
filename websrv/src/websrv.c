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

int runServer()
{
	int server_socket = init_server_socket(DEFAULT_SERVER_PORT);
	if (server_socket < 0)
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
	result = start_worker();
	if (result == -1)
	{
		perror("Error creating worker.");
		return -1;
	}

	process_incoming_connections(server_socket);

	stop_worker();
	destroy_context_storage();
	pthread_exit(EXIT_SUCCESS);
}

int main(void)
{
//	return test_storage();
	return runServer();
}
