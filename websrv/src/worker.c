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

void set_socket_write_mode(struct SocketContext* pSc)
{
	struct epoll_event ev;

	ev.events = EPOLLIN | EPOLLOUT;
	ev.data.u64 = 0LL;
	ev.data.fd = pSc->client_socket;
	if (epoll_ctl(pSc->epoll_fd, EPOLL_CTL_MOD, pSc->client_socket, &ev) < 0)
	{
		fprintf(stderr, "Couldn't modify client socket %d in epoll set: %m\n",
				pSc->client_socket);
	}
}

static void * worker_thread(void * p)
{
	struct SocketContext* pSc = (struct SocketContext*) p;

	printf("Socket=%d; Request [%s]\n", pSc->client_socket, pSc->pRequest);

	char* response = "Response\n";

	pSc->pResponse = malloc(strlen(response) + sizeof(char));
	strcpy(pSc->pResponse, response);

	set_socket_write_mode(pSc);

	pthread_detach(pthread_self());
	return (p);
}
