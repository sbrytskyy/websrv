/*
 * server.c
 *
 *  Created on: May 5, 2016
 *      Author: sbrytskyy
 */

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#include "storage.h"
#include "server.h"

#define EPOLL_ARRAY_SIZE   64

int epoll_fd;

int read_incoming_data(int client_socket)
{
	char buffer[MAX_PACKET_SIZE];

	int nbytes = read(client_socket, buffer, 512);
	if (nbytes < 0)
	{
		perror("Error reading socket: %m");
		return -1;
	}
	else if (nbytes == 0)
	{
		return -1;
	}
	else
	{
		buffer[nbytes] = '\0';
		printf("Received message [%s], length: %d\n", buffer, nbytes);

		struct SocketContext* pSc = malloc(sizeof(struct SocketContext));
		pSc->client_socket = client_socket;
		pSc->pRequest = malloc(nbytes);
		strcpy(pSc->pRequest, buffer);

		add_input(pSc);

		return 0;
	}
}

int set_socket_write_mode(int client_socket)
{
	struct epoll_event ev;

	ev.events = EPOLLIN | EPOLLOUT;
	ev.data.u64 = 0LL;
	ev.data.fd = client_socket;

	int result = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_socket, &ev);

	if (result < 0)
	{
		fprintf(stderr, "Couldn't modify client socket %d in epoll set: %m\n",
				client_socket);
	}

	return result;
}

int write_response(int client_socket)
{
	printf("[write_response] socket=%d\n", client_socket);

	struct SocketContext* pSc = get_output(client_socket);

	char* response = pSc->pResponse;
	printf("[write_response] [%s]\n", response);

	int result = send(client_socket, response, strlen(response), 0);

	free(pSc->pResponse);
	free(pSc->pRequest);
	free(pSc);

	return result;
}

void close_handle(int handle)
{
	shutdown(handle, SHUT_RDWR);
	close(handle);
}

int init_server_socket(uint16_t port)
{
	int handle;
	int on = 1;
	struct sockaddr_in serveraddr;

	handle = socket(PF_INET, SOCK_STREAM, 0);
	if (handle < 0)
	{
		perror("Error creating socket: %m");
		return -1;
	}

	if (fcntl(handle, F_SETFL, O_NONBLOCK))
	{
		perror("Could not make the socket non-blocking: %m\n");
		close(handle);
		return -1;
	}

	if (setsockopt(handle, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)))
	{
		fprintf(stderr, "Could not set socket %d option for reusability: %m\n",
				handle);
		close(handle);
		return -1;
	}

	memset(&serveraddr, 0, sizeof(serveraddr));

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(port);
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(handle, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
	{
		perror("Error binding socket: %m");
		return -1;
	}
	else
	{
		printf("Bound socket %d to address 'INADDR_ANY' and port %u\n", handle,
				port);
	}

	if (listen(handle, SOMAXCONN))
	{
		fprintf(stderr, "Could not start listening on server socket %d: %m\n",
				handle);
		close_handle(handle);
		return -1;
	}
	else
	{
		printf(
				"Server socket %d started listening to address 'INADDR_ANY' and port %u\n",
				handle, port);
	}

	return handle;
}

int process_incoming_connections(int server_socket)
{
	int result;
	int pollsize = 1;

	struct epoll_event ev;
	struct epoll_event epoll_events[EPOLL_ARRAY_SIZE];

	epoll_fd = epoll_create(pollsize);

	if (epoll_fd < 0)
	{
		perror("Could not create the epoll of file descriptors: %m");
		close_handle(server_socket);
		return 1;
	}

	ev.events = EPOLLIN;
	ev.data.u64 = 0LL;
	ev.data.fd = server_socket;

	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &ev) < 0)
	{
		fprintf(stderr, "Couldn't add server socket %d to epoll set: %m\n",
				server_socket);
		close_handle(server_socket);
		return -1;
	}

	while (1)
	{
		printf("Starting epoll_wait on %d file descriptors\n", pollsize);

		while ((result = epoll_wait(epoll_fd, epoll_events, EPOLL_ARRAY_SIZE,
				-1)) < 0)
		{
			if ((result < 0) && (errno != EINTR))
			{
				fprintf(stderr, "EPoll on %d file descriptors failed: %m\n",
						pollsize);
				close_handle(server_socket);
				return -1;
			}
		}

		for (int i = 0; i < result; i++)
		{
			uint32_t events = epoll_events[i].events;
			int handle = epoll_events[i].data.fd;

			if ((events & EPOLLERR) || (events & EPOLLHUP)
					|| (events & EPOLLRDHUP))
			{
				if (handle == server_socket)
				{
					fprintf(stderr, "EPoll on %d file descriptors failed: %m\n",
							pollsize);
					close_handle(server_socket);
					return -1;
				}
				else
				{
					printf("Closing socket with handle %d\n", handle);
					close_handle(handle);
					continue;
				}
			}
			else if (events & EPOLLIN)
			{
				if (handle == server_socket)
				{
					int client_socket;
					struct sockaddr_in clientaddr;
					socklen_t clientaddr_size = sizeof(clientaddr);
					char buffer[1024];

					while ((client_socket = accept(server_socket,
							(struct sockaddr *) &clientaddr, &clientaddr_size))
							< 0)
					{
						if ((client_socket < 0) && (errno != EINTR))
						{
							fprintf(stderr, "Accept on socket %d failed: %m\n",
									server_socket);
							close_handle(server_socket);
							return -1;
						}
					}

					if (inet_ntop(AF_INET, &clientaddr.sin_addr.s_addr, buffer,
							sizeof(buffer)) != NULL)
					{
						printf(
								"[Web Server] Accepted connection from %s:%u, assigned new sd %d\n",
								buffer, ntohs(clientaddr.sin_port),
								client_socket);
					}
					else
					{
						perror(
								"Failed to convert address from binary to text form: %m\n");
					}

					ev.events = EPOLLIN;
					ev.data.u64 = 0LL;
					ev.data.fd = client_socket;

					if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &ev)
							< 0)
					{
						fprintf(
						stderr,
								"Couldn't add client socket %d to epoll set: %m\n",
								client_socket);
						close_handle(server_socket);
						return -1;
					}

					pollsize++;
				}
				else
				{
					if (read_incoming_data(handle) < 0)
					{
						fprintf(stderr, "Receive from socket %d failed: %m\n",
								handle);
						pollsize--;
						close_handle(handle);
						continue;
					}
				}
			}
			else if (events & EPOLLOUT)
			{
				if (handle != server_socket)
				{
					int nsent = write_response(handle);
					printf("Sent %d bytes as response.\n", nsent);
					if (nsent <= 0)
					{
						close_handle(handle);
						continue;
					}
					else
					{
						ev.events = EPOLLIN;
						ev.data.u64 = 0LL;
						ev.data.fd = handle;

						if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, handle, &ev)
								< 0)
						{
							printf(
									"Couldn't modify client socket %d in epoll set: %m\n",
									handle);
							close_handle(server_socket);
						}
					}
				}
			}
		}
	}

	close_handle(server_socket);

	return 0;
}
