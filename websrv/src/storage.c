/*
 * storage.c
 *
 *  Created on: May 5, 2016
 *      Author: sbrytskyy
 */

#include "storage.h"

#include <stdio.h>
#include <stdlib.h>

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

	return 0;
}

int add_input(struct SocketContext* pSc)
{
	return add(pInQueue, pSc);
}

int add_output(struct SocketContext* pSc)
{
	return add(pOutList, pSc);
}

struct SocketContext* get_first_input()
{
	struct ListItem* pItem = pInQueue->pFirst;
	if (pItem != NULL)
	{
		delete(pInQueue, pItem->pSc);
		return pItem->pSc;
	}

	return NULL;
}

struct SocketContext* get_output(int sock)
{
	struct ListItem* pItem = find(pOutList, sock);
	if (pItem != NULL)
	{
		delete(pOutList, pItem->pSc);
		return pItem->pSc;
	}

	return NULL;
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
