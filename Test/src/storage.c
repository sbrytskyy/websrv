/*
 * storage.c
 *
 *  Created on: May 5, 2016
 *      Author: sbrytskyy
 */

#include "storage.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#define STORAGE_CACHE_SIZE 32

struct socket_context* incoming_queue;
struct socket_context* output_list;

int add(struct socket_context* storage, struct socket_context* sc);
struct socket_context* find(struct socket_context* storage, int sock);

pthread_mutex_t inqueue_mutex;
pthread_cond_t empty_inqueue_cv;

pthread_mutex_t outqueue_mutex;
pthread_cond_t empty_outqueue_cv;

int init_storage()
{
	pthread_mutex_init(&inqueue_mutex, NULL);
	pthread_cond_init(&empty_inqueue_cv, NULL);

	pthread_mutex_init(&outqueue_mutex, NULL);
	pthread_cond_init(&empty_outqueue_cv, NULL);

	return 0;
}

int cleanup_storage()
{
	pthread_mutex_destroy(&inqueue_mutex);
	pthread_cond_destroy(&empty_inqueue_cv);

	pthread_mutex_destroy(&outqueue_mutex);
	pthread_cond_destroy(&empty_outqueue_cv);

	return 0;
}

struct socket_context* create_socket_context(int client_socket, char* buffer)
{
	// todo rework using pool of objects

	struct socket_context* sc = malloc(sizeof(struct socket_context));
	if (sc != NULL)
	{
		sc->client_socket = client_socket;
		sc->request = malloc(MAX_PACKET_SIZE + 1);
		if (sc->request == NULL)
		{
			free(sc);
			return NULL;
		}
		strcpy(sc->request, buffer);
		sc->response = NULL;
		sc->close_after_response = 0;
	}

	return sc;
}

void destroy_socket_context(struct socket_context* sc)
{
	if (sc->response)
	{
		free(sc->response);
	}
	if (sc->request)
	{
		free(sc->request);
	}
	free(sc);
}

int add_input(struct socket_context* sc)
{
	pthread_mutex_lock(&inqueue_mutex);

	int result = 0;
	if (incoming_queue == NULL)
	{
		incoming_queue = sc;
	}
	else
	{
		result = add(incoming_queue, sc);
	}

	if (result != -1)
	{
		pthread_cond_broadcast(&empty_inqueue_cv);
	}

	pthread_mutex_unlock(&inqueue_mutex);

	return result;
}

int add_output(struct socket_context* sc)
{
	pthread_mutex_lock(&outqueue_mutex);

	int result = 0;
	if (output_list == NULL)
	{
		output_list = sc;
	}
	else
	{
		result = add(output_list, sc);
	}

	if (result != -1)
	{
		pthread_cond_broadcast(&empty_outqueue_cv);
	}

	pthread_mutex_unlock(&outqueue_mutex);

	return result;
}

struct socket_context* poll_first_input()
{
	pthread_mutex_lock(&inqueue_mutex);

	while (incoming_queue == NULL)
	{
		pthread_cond_wait(&empty_inqueue_cv, &inqueue_mutex);
	}

	struct socket_context* sc = incoming_queue;

	incoming_queue = sc->next;
	pthread_mutex_unlock(&inqueue_mutex);

	return sc;
}

struct socket_context* get_output(int client_socket)
{
	pthread_mutex_lock(&outqueue_mutex);

	struct socket_context* sc = find(output_list, client_socket);
	if (sc != NULL)
	{
		output_list = sc->next;
	}

	pthread_mutex_unlock(&outqueue_mutex);

	return sc;
}

/******************************************************************/

int add(struct socket_context* storage, struct socket_context* sc)
{
	struct socket_context* existing = find(storage, sc->client_socket);
	if (existing == NULL)
	{
		struct socket_context* tp = storage;
		while (tp->next != NULL)
		{
			tp = tp->next;
		}

		tp->next = sc;
		sc->next = NULL;

		return 0;
	}
	fprintf(stderr, "[error] Socket Context already exists: socket=%d.\n", sc->client_socket);

	return -1;
}

struct socket_context* find(struct socket_context* storage, int sock)
{
	struct socket_context* tp = storage;

	while (tp)
	{
		if (tp->client_socket == sock)
		{
			return tp;
		}
		tp = tp->next;
	}

	return NULL;
}
