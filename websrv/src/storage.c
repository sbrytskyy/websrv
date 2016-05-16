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
	struct list_item* pFirst;
	struct list_item* pLast;
};

struct list_item
{
	struct list_item* pPrev;
	struct list_item* pNext;

	struct socket_context* pSc;
};

struct storage* incoming_queue;
struct storage* output_list;

int add(struct storage* pStorage, struct socket_context* pSc);
int delete(struct storage* pStorage, struct socket_context* pSc);
struct list_item* find(struct storage* pStorage, int sock);

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
	struct list_item* pItem = incoming_queue->pFirst;
	while (pItem)
	{
		struct list_item* pItem2Delete = pItem;
		destroy_context_storage(pItem2Delete->pSc);
		free(pItem2Delete);
		pItem = pItem->pNext;
	}
	free(incoming_queue);

	pItem = output_list->pFirst;
	while (pItem)
	{
		struct list_item* pItem2Delete = pItem;
		destroy_context_storage(pItem2Delete->pSc);
		free(pItem2Delete);
		pItem = pItem->pNext;
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

	struct socket_context* pSc = malloc(sizeof(struct socket_context));
	if (pSc != NULL)
	{
		pSc->client_socket = client_socket;
		pSc->pRequest = malloc(strlen(buffer));
		if (pSc->pRequest == NULL)
		{
			free(pSc);
			return NULL;
		}
		strcpy(pSc->pRequest, buffer);
		pSc->pResponse = NULL;
		pSc->close_after_response = 0;
	}

	return pSc;
}

void destroy_socket_context(struct socket_context* pSc)
{
	if (pSc->pResponse)
	{
		free(pSc->pResponse);
	}
	free(pSc->pRequest);
	free(pSc);
}

int add_input(struct socket_context* pSc)
{
	pthread_mutex_lock(&inqueue_mutex);

	int result = add(incoming_queue, pSc);
	if (result != -1)
	{
		pthread_cond_signal(&empty_inqueue_cv);
	}

	pthread_mutex_unlock(&inqueue_mutex);

	return result;
}

int add_output(struct socket_context* pSc)
{
	pthread_mutex_lock(&outqueue_mutex);

	int result = add(output_list, pSc);
	if (result != -1)
	{
		pthread_cond_signal(&empty_outqueue_cv);
	}

	pthread_mutex_unlock(&outqueue_mutex);

	return result;
}

struct socket_context* poll_first_input()
{
	struct socket_context* pSc = NULL;

	pthread_mutex_lock(&inqueue_mutex);

	while (incoming_queue->pFirst == NULL)
	{
		pthread_cond_wait(&empty_inqueue_cv, &inqueue_mutex);
	}

	struct list_item* pItem = incoming_queue->pFirst;
	if (pItem != NULL)
	{
		delete(incoming_queue, pItem->pSc);
		pSc = pItem->pSc;
	}
	pthread_mutex_unlock(&inqueue_mutex);

	return pSc;
}

struct socket_context* get_first_input()
{
	struct socket_context* pSc = NULL;

	pthread_mutex_lock(&inqueue_mutex);

	struct list_item* pItem = incoming_queue->pFirst;
	if (pItem != NULL)
	{
		delete(incoming_queue, pItem->pSc);
		pSc = pItem->pSc;
	}
	pthread_mutex_unlock(&inqueue_mutex);

	return pSc;
}

struct socket_context* poll_output(int client_socket)
{
	struct socket_context* pSc = NULL;

	pthread_mutex_lock(&outqueue_mutex);

	struct list_item* pItem;
	do
	{
		pItem = find(output_list, client_socket);
		if (pItem != NULL)
		{
			delete(output_list, pItem->pSc);
			pSc = pItem->pSc;
			break;
		}

		pthread_cond_wait(&empty_outqueue_cv, &outqueue_mutex);
	} while (pItem == NULL);

	pthread_mutex_unlock(&outqueue_mutex);

	return pSc;
}

struct socket_context* get_output(int client_socket)
{
	struct socket_context* pSc = NULL;

	pthread_mutex_lock(&outqueue_mutex);

	struct list_item* pItem = find(output_list, client_socket);
	if (pItem != NULL)
	{
		delete(output_list, pItem->pSc);
		pSc = pItem->pSc;
	}

	pthread_mutex_unlock(&outqueue_mutex);

	return pSc;
}

/******************************************************************/

int add(struct storage* pStorage, struct socket_context* pSc)
{
	struct list_item* pItem = find(pStorage, pSc->client_socket);
	if (pItem == NULL)
	{
		pItem = malloc(sizeof(struct list_item));
		if (pItem == NULL)
		{
			return -1;
		}
		pItem->pSc = pSc;
		pItem->pNext = NULL;
		pItem->pPrev = NULL;

		if (pStorage->pLast == NULL)
		{
			pStorage->pFirst = pItem;
			pStorage->pLast = pItem;
		}
		else
		{
			pStorage->pLast->pNext = pItem;
			pItem->pPrev = pStorage->pLast;

			pStorage->pLast = pItem;
		}

		return 0;
	}
	fprintf(stderr, "[error] Socket Context already exists: socket=%d.\n", pSc->client_socket);

	return -1;
}

int delete(struct storage* pStorage, struct socket_context* pSc)
{
	struct list_item* pItem = pStorage->pFirst;

	while (pItem)
	{
		if (pItem->pSc == pSc)
		{
			if (pItem->pPrev != NULL)
			{
				pItem->pPrev->pNext = pItem->pNext;
			}
			else
			{
				pStorage->pFirst = pItem->pNext;
			}

			if (pItem->pNext != NULL)
			{
				pItem->pNext->pPrev = pItem->pPrev;
			}
			else
			{
				pStorage->pLast = pItem->pPrev;
			}

			free(pItem);

			return 0;
		}
		pItem = pItem->pNext;
	}

	return -1;
}

struct list_item* find(struct storage* pStorage, int sock)
{
	struct list_item* pItem = pStorage->pFirst;

	while (pItem)
	{
		if (pItem->pSc->client_socket == sock)
		{
			return pItem;
		}
		pItem = pItem->pNext;
	}

	return NULL;
}
