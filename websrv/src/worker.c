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
	struct SocketData* pSd = (struct SocketData*)p;

	printf("Socket=%d; Request [%s]\n", pSd->client_socket, pSd->pCharData);

	FD_SET(pSd->client_socket, pSd->write_fd_set);

	free(pSd->pCharData);
	free(p);

    pthread_detach(pthread_self());
    return (p);
}
