/*
 * storage.h
 *
 *  Created on: May 3, 2016
 *      Author: sbrytskyy
 */

#ifndef STRUCTURES_H_
#define STRUCTURES_H_

#define MAX_PACKET_SIZE	8800  /* FF3=4096, IE7=8760 */


struct socket_context
{
	struct socket_context* next;

	int client_socket;
	char* request;
	char* response;
	int response_len;
	int close_after_response;
};

int init_storage();
int cleanup_storage();

struct socket_context* create_socket_context(int client_socket, char* buffer);
void destroy_socket_context(struct socket_context* sc);

int add_input(struct socket_context* sc);
int add_output(struct socket_context* sc);
struct socket_context* poll_first_input();
struct socket_context* get_first_input();
struct socket_context* get_output(int client_socket);
struct socket_context* poll_output(int client_socket);

#endif /* STRUCTURES_H_ */
