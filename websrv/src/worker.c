/*
 * worker.c
 *
 *  Created on: May 5, 2016
 *      Author: sbrytskyy
 */

#include "worker.h"

static void * worker_thread(void *);

void start_worker(void * arg)
{
    pthread_t worker;
    int err = pthread_create(&worker, NULL, worker_thread, arg);
    if (err != 0)
    {
    	printf("can't create thread :[%s]\n", strerror(err));
    }
}

static void * worker_thread(void * p)
{
	struct SocketContext* pSc = (struct SocketContext*)p;

	printf("Socket=%d; Request [%s]\n", pSc->client_socket, pSc->pRequest);

	char* response = "Response\n";

	pSc->pResponse = malloc(strlen(response) + sizeof(char));
	strcpy(pSc->pResponse, response);

	FD_SET(pSc->client_socket, pSc->write_fd_set);

    pthread_detach(pthread_self());
    return (p);
}
