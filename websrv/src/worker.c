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

static void * worker_thread(void *);

void start_worker(void * arg)
{
	pthread_t worker;
	int err = pthread_create(&worker, NULL, worker_thread, arg);
	if (err != 0)
	{
		fprintf(stderr, "can't create thread :[%s]\n", strerror(err));
	}
}

static void * worker_thread(void * p)
{
	while (1)
	{
		struct SocketContext* pSc = get_first_input();

		if (pSc != NULL)
		{
			printf("Socket=%d; Request [%s]\n", pSc->client_socket,
					pSc->pRequest);

			//char* response = "HTTP/1.1 200 OK\n<html>\n<body>\n<h1>Hello, World!</h1>\n</body>\n</html>";
			char* response = "Response\n";

			pSc->pResponse = malloc(strlen(response) + sizeof(char));
			strcpy(pSc->pResponse, response);

			add_output(pSc);
			set_socket_write_mode(pSc->client_socket);
		}
		else
		{
			// todo Refactor using synchronization
			usleep(100000);
		}
	}

	pthread_detach(pthread_self());
	return (NULL);
}
