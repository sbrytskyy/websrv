/*
 * structures.c
 *
 *  Created on: May 5, 2016
 *      Author: sbrytskyy
 */


#include "structures.h"

struct OutData* pOut;

struct ListItem* find(int sock);

int init_context_storage()
{
	pOut = malloc(sizeof(struct OutData));
	if (pOut == NULL)
	{
		return -1;
	}

	return 0;
}

int store_socket_context(struct SocketContext* pSc)
{
	struct ListItem* pItem = find(pSc->client_socket);
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

		if (pOut->pLast == NULL)
		{
			pOut->pFirst = pItem;
			pOut->pLast = pItem;
		}
		else
		{
			pOut->pLast->pNext = pItem;
			pItem->pPrev = pOut->pLast;

			pOut->pLast = pItem;
		}

		return 0;
	}

	printf("[error] Socket Context already exists for output.");

	return -1;
}


int remove_socket_context(struct SocketContext* pSc)
{
	struct ListItem* pItem = pOut->pFirst;

	while (pItem)
	{
		if (pItem->pSc == pSc)
		{
			if (pItem->pPrev != NULL)
			{
				pItem->pPrev->pNext = pItem->pNext;
			}
			if (pItem->pNext != NULL)
			{
				pItem->pNext->pPrev = pItem->pPrev;
			}

			free(pSc->pResponse);
			free(pSc->pRequest);
			free(pSc);
			free(pItem);

			return 0;
		}
		pItem = pItem->pNext;
	}

	return -1;
}


struct ListItem* find(int sock)
{
	struct ListItem* pItem = pOut->pFirst;

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

struct SocketContext* get_socket_context(int sock)
{
	struct ListItem* pItem = find(sock);
	if (pItem != NULL)
	{
		return pItem->pSc;
	}

	return NULL;
}
