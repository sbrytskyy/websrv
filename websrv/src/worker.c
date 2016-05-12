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

static void * worker_thread(void *);

int thread_state = 0; // 0: normal, -1: stop thread, 1: do something

void start_worker()
{
	pthread_t worker;
	int err = pthread_create(&worker, NULL, worker_thread,
			(void*) &thread_state);
	if (err != 0)
	{
		fprintf(stderr, "can't create thread :[%s]\n", strerror(err));
	}
}

void stop_worker()
{
	thread_state = 1;
}

static void * worker_thread(void * p)
{
	int* pthread_state = p;

	while (1)
	{
		struct SocketContext* pSc = poll_first_input();

		if (pSc != NULL)
		{
//			printf("Socket=%d; Request [%s]\n", pSc->client_socket,
//					pSc->pRequest);

			process_http(pSc);

			add_output(pSc);
			set_socket_write_mode(pSc->client_socket);
		}

		if (*pthread_state == -1)
		{
			break;
		}
	}

	pthread_exit(NULL);
	return (NULL);
}
