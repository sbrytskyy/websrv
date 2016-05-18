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

struct storage
{
	struct list_item* first;
	struct list_item* last;
};

struct list_item
{
	struct list_item* prev;
	struct list_item* next;

	struct socket_context* sc;
};

struct storage* incoming_queue;
struct storage* output_list;

int add(struct storage* storage, struct socket_context* sc);
int delete(struct storage* storage, struct socket_context* sc);
struct list_item* find(struct storage* storage, int sock);

pthread_mutex_t inqueue_mutex;
pthread_cond_t empty_inqueue_cv;

pthread_mutex_t outqueue_mutex;
pthread_cond_t empty_outqueue_cv;

int init_context_storage()
{
	incoming_queue = malloc(sizeof(struct storage));
	if (incoming_queue == NULL)
	{
		return -1;
	}

	output_list = malloc(sizeof(struct storage));
	if (output_list == NULL)
	{
		free(incoming_queue);
		return -1;
	}

	pthread_mutex_init(&inqueue_mutex, NULL);
	pthread_cond_init(&empty_inqueue_cv, NULL);

	pthread_mutex_init(&outqueue_mutex, NULL);
	pthread_cond_init(&empty_outqueue_cv, NULL);

	return 0;
}

int destroy_context_storage()
{
	struct list_item* pItem = incoming_queue->first;
	while (pItem)
	{
		struct list_item* pItem2Delete = pItem;
		destroy_context_storage(pItem2Delete->sc);
		free(pItem2Delete);
		pItem = pItem->next;
	}
	free(incoming_queue);

	pItem = output_list->first;
	while (pItem)
	{
		struct list_item* pItem2Delete = pItem;
		destroy_context_storage(pItem2Delete->sc);
		free(pItem2Delete);
		pItem = pItem->next;
	}
	free(output_list);

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
		sc->request = malloc(strlen(buffer));
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
	free(sc->request);
	free(sc);
}

int add_input(struct socket_context* sc)
{
	pthread_mutex_lock(&inqueue_mutex);

	int result = add(incoming_queue, sc);
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

	int result = add(output_list, sc);
	if (result != -1)
	{
		pthread_cond_broadcast(&empty_outqueue_cv);
	}

	pthread_mutex_unlock(&outqueue_mutex);

	return result;
}

struct socket_context* poll_first_input()
{
	struct socket_context* sc = NULL;

	pthread_mutex_lock(&inqueue_mutex);

	while (incoming_queue->first == NULL)
	{
		pthread_cond_wait(&empty_inqueue_cv, &inqueue_mutex);
	}

	struct list_item* pItem = incoming_queue->first;
	if (pItem != NULL)
	{
		sc = pItem->sc;
		delete(incoming_queue, pItem->sc);
	}
	pthread_mutex_unlock(&inqueue_mutex);

	return sc;
}

struct socket_context* get_first_input()
{
	struct socket_context* sc = NULL;

	pthread_mutex_lock(&inqueue_mutex);

	struct list_item* pItem = incoming_queue->first;
	if (pItem != NULL)
	{
		sc = pItem->sc;
		delete(incoming_queue, pItem->sc);
	}
	pthread_mutex_unlock(&inqueue_mutex);

	return sc;
}

struct socket_context* poll_output(int client_socket)
{
	struct socket_context* sc = NULL;

	pthread_mutex_lock(&outqueue_mutex);

	struct list_item* pItem;
	do
	{
		pItem = find(output_list, client_socket);
		if (pItem != NULL)
		{
			sc = pItem->sc;
			delete(output_list, pItem->sc);
			break;
		}

		pthread_cond_wait(&empty_outqueue_cv, &outqueue_mutex);
	} while (pItem == NULL);

	pthread_mutex_unlock(&outqueue_mutex);

	return sc;
}

struct socket_context* get_output(int client_socket)
{
	struct socket_context* sc = NULL;

	pthread_mutex_lock(&outqueue_mutex);

	struct list_item* pItem = find(output_list, client_socket);
	if (pItem != NULL)
	{
		sc = pItem->sc;
		delete(output_list, pItem->sc);
	}

	pthread_mutex_unlock(&outqueue_mutex);

	return sc;
}

/******************************************************************/

int add(struct storage* storage, struct socket_context* sc)
{
	struct list_item* pItem = find(storage, sc->client_socket);
	if (pItem == NULL)
	{
		pItem = malloc(sizeof(struct list_item));
		if (pItem == NULL)
		{
			return -1;
		}
		pItem->sc = sc;
		pItem->next = NULL;
		pItem->prev = NULL;

		if (storage->last == NULL)
		{
			storage->first = pItem;
			storage->last = pItem;
		}
		else
		{
			storage->last->next = pItem;
			pItem->prev = storage->last;

			storage->last = pItem;
		}

		return 0;
	}
	fprintf(stderr, "[error] Socket Context already exists: socket=%d.\n", sc->client_socket);

	return -1;
}

int delete(struct storage* storage, struct socket_context* sc)
{
	struct list_item* pItem = storage->first;

	while (pItem)
	{
		if (pItem->sc == sc)
		{
			if (pItem->prev != NULL)
			{
				pItem->prev->next = pItem->next;
			}
			else
			{
				storage->first = pItem->next;
			}

			if (pItem->next != NULL)
			{
				pItem->next->prev = pItem->prev;
			}
			else
			{
				storage->last = pItem->prev;
			}

			free(pItem);

			return 0;
		}
		pItem = pItem->next;
	}

	return -1;
}

struct list_item* find(struct storage* storage, int sock)
{
	struct list_item* pItem = storage->first;

	while (pItem)
	{
		if (pItem->sc->client_socket == sock)
		{
			return pItem;
		}
		pItem = pItem->next;
	}

	return NULL;
}
