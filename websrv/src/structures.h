/*
 * structures.h
 *
 *  Created on: May 3, 2016
 *      Author: sbrytskyy
 */

#ifndef STRUCTURES_H_
#define STRUCTURES_H_


#include <stdio.h>
#include <stdlib.h>


struct OutData
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

struct SocketContext
{
	int client_socket;
	char* pRequest;
	char* pResponse;
	fd_set* write_fd_set;
};


int init_context_storage();
int store_socket_context(struct SocketContext* pSc);
struct SocketContext* get_socket_context(int sock);
int remove_socket_context(struct SocketContext* pSc);

#endif /* STRUCTURES_H_ */
