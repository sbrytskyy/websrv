/*
 * storage.h
 *
 *  Created on: May 3, 2016
 *      Author: sbrytskyy
 */

#ifndef STRUCTURES_H_
#define STRUCTURES_H_

struct SocketContext
{
	int client_socket;
	char* pRequest;
	char* pResponse;
	int close_after_response;
};

int init_context_storage();
int destroy_context_storage();

struct SocketContext* create_socket_context(int client_socket, char* buffer);
void destroy_socket_context(struct SocketContext* pSc);

int add_input(struct SocketContext* pSc);
int add_output(struct SocketContext* pSc);
struct SocketContext* get_first_input();
struct SocketContext* get_output(int sock);

#endif /* STRUCTURES_H_ */
