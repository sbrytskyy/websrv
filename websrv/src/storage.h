/*
 * storage.h
 *
 *  Created on: May 3, 2016
 *      Author: sbrytskyy
 */

#ifndef STRUCTURES_H_
#define STRUCTURES_H_

struct socket_context
{
	int client_socket;
	char* pRequest;
	char* pResponse;
	int close_after_response;
};

int init_context_storage();
int destroy_context_storage();

struct socket_context* create_socket_context(int client_socket, char* buffer);
void destroy_socket_context(struct socket_context* pSc);

int add_input(struct socket_context* pSc);
int add_output(struct socket_context* pSc);
struct socket_context* poll_first_input();
struct socket_context* get_first_input();
struct socket_context* get_output(int client_socket);
struct socket_context* poll_output(int client_socket);

#endif /* STRUCTURES_H_ */
