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

struct Storage
{
	struct ListItem* pFirst;
	struct ListItem* pLast;
};

struct ListItem
{
	struct ListItem* pPrev;
	struct ListItem* pNext;

	struct SocketContext* pSc;
};

struct Storage* pInQueue;
struct Storage* pOutList;

int add(struct Storage* pStorage, struct SocketContext* pSc);
int delete(struct Storage* pStorage, struct SocketContext* pSc);
struct ListItem* find(struct Storage* pStorage, int sock);

pthread_mutex_t inqueue_mutex;
pthread_cond_t empty_inqueue_cv;

int init_context_storage()
{
	pInQueue = malloc(sizeof(struct Storage));
	if (pInQueue == NULL)
	{
		return -1;
	}

	pOutList = malloc(sizeof(struct Storage));
	if (pOutList == NULL)
	{
		return -1;
	}

	pthread_mutex_init(&inqueue_mutex, NULL);
	pthread_cond_init(&empty_inqueue_cv, NULL);

	return 0;
}

int destroy_context_storage()
{
	struct ListItem* pItem = pInQueue->pFirst;
	while (pItem)
	{
		struct ListItem* pItem2Delete = pItem;
		destroy_context_storage(pItem2Delete->pSc);
		free(pItem2Delete);
		pItem = pItem->pNext;
	}
	free(pInQueue);

	pItem = pOutList->pFirst;
	while (pItem)
	{
		struct ListItem* pItem2Delete = pItem;
		destroy_context_storage(pItem2Delete->pSc);
		free(pItem2Delete);
		pItem = pItem->pNext;
	}
	free(pOutList);

	pthread_mutex_destroy(&inqueue_mutex);
	pthread_cond_destroy(&empty_inqueue_cv);

	return 0;
}

struct SocketContext* create_socket_context(int client_socket, char* buffer)
{
	struct SocketContext* pSc = malloc(sizeof(struct SocketContext));
	pSc->client_socket = client_socket;
	pSc->pRequest = malloc(strlen(buffer));
	strcpy(pSc->pRequest, buffer);
	pSc->pResponse = NULL;
	pSc->close_after_response = 0;

	return pSc;
}

void destroy_socket_context(struct SocketContext* pSc)
{
	if (pSc->pResponse)
	{
		free(pSc->pResponse);
	}
	free(pSc->pRequest);
	free(pSc);
}

int add_input(struct SocketContext* pSc)
{
	pthread_mutex_lock(&inqueue_mutex);

	int result = add(pInQueue, pSc);

	pthread_cond_signal(&empty_inqueue_cv);

	pthread_mutex_unlock(&inqueue_mutex);

	return result;
}

int add_output(struct SocketContext* pSc)
{
	return add(pOutList, pSc);
}

struct SocketContext* poll_first_input()
{
	struct SocketContext* pSc = NULL;

	pthread_mutex_lock(&inqueue_mutex);

	while (pInQueue->pFirst == NULL)
	{
		pthread_cond_wait(&empty_inqueue_cv, &inqueue_mutex);
	}

	struct ListItem* pItem = pInQueue->pFirst;
	if (pItem != NULL)
	{
		delete(pInQueue, pItem->pSc);
		pSc = pItem->pSc;
	}
	pthread_mutex_unlock(&inqueue_mutex);

	return pSc;
}

struct SocketContext* get_first_input()
{
	struct SocketContext* pSc = NULL;

	pthread_mutex_lock(&inqueue_mutex);

	struct ListItem* pItem = pInQueue->pFirst;
	if (pItem != NULL)
	{
		delete(pInQueue, pItem->pSc);
		pSc = pItem->pSc;
	}
	pthread_mutex_unlock(&inqueue_mutex);

	return pSc;
}

struct SocketContext* get_output(int sock)
{
	struct SocketContext* pSc = NULL;

	struct ListItem* pItem = find(pOutList, sock);
	if (pItem != NULL)
	{
		delete(pOutList, pItem->pSc);
		pSc = pItem->pSc;
	}

	return pSc;
}

/******************************************************************/

int add(struct Storage* pStorage, struct SocketContext* pSc)
{
	struct ListItem* pItem = find(pStorage, pSc->client_socket);
	if (pItem == NULL)
	{
		pItem = malloc(sizeof(struct ListItem));
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
	printf("[error] Socket Context already exists for output.");

	return -1;
}

int delete(struct Storage* pStorage, struct SocketContext* pSc)
{
	struct ListItem* pItem = pStorage->pFirst;

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

struct ListItem* find(struct Storage* pStorage, int sock)
{
	struct ListItem* pItem = pStorage->pFirst;

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
