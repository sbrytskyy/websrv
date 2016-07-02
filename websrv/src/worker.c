/*
 * worker.c
 *
 *  Created on: May 5, 2016
 *      Author: sbrytskyy
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "storage.h"
#include "server.h"
#include "protocol.h"
#include "utils.h"

static void * worker_thread(void *);
#define NUMBER_OF_WORKERS 3

volatile int thread_state = 0; // 0: normal, -1: stop thread, 1: do something

int root_folder_name_len;
const char* root_folder_name;

int start_workers()
{
	root_folder_name = get_root_dir();
	root_folder_name_len = strlen(root_folder_name);

	for (int i = 0; i < NUMBER_OF_WORKERS; i++)
	{
		pthread_t worker;
		int rc = pthread_create(&worker, NULL, worker_thread,
				(void*) &thread_state);
		if (rc)
		{
			fprintf(stderr,
					"ERROR; return code from %d pthread_create() is %d\n", i,
					rc);

			if (i == 0)
			{
				fprintf(stderr, "ERROR; No workers have been created.\n");
				return -1;
			}
		}
	}

	return 0;
}

void stop_workers()
{
	thread_state = 1;
}

static void * worker_thread(void * p)
{
	int* state = p;

	char* working_path = malloc(PATH_MAX + 1);

	if (working_path != NULL)
	{
		if (strcpy(working_path, root_folder_name) != NULL)
		{
			while (1)
			{
				struct socket_context* sc = poll_first_input();

				if (sc != NULL)
				{
					dprint("Socket=%d; Request [%s]\n", sc->client_socket,
							sc->request);

					working_path[root_folder_name_len] = '\0';

					// todo if process_http returns -1 instead of adding to response queue, just remove it and close socket as error.
					process_http(sc, working_path);

					add_output(sc);
					set_socket_write_mode(sc->client_socket);
				}

				if (*state == -1)
				{
					break;
				}
			}
		}

		free(working_path);
	}

	pthread_exit(NULL);
}
