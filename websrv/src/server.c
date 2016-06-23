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
#include "utils.h"

#define EPOLL_ARRAY_SIZE   64

int epoll_fd;

// todo rework buffer handling
char buffer[MAX_PACKET_SIZE];

int read_incoming_data(int client_socket)
{
	int nbytes = read(client_socket, buffer, sizeof(buffer));
	if (nbytes < 0)
	{
		fprintf(stderr, "Error reading socket %d: %m\n", client_socket);
	}
	else if (nbytes == 0)
	{
		dprint("Socket %d has been closed by client: %m\n", client_socket);
	}
	else
	{
		buffer[nbytes] = '\0';
		dprint("Received message [%s], length: %d\n\n", buffer, nbytes);
		dprint("Received bytes: %d\n", nbytes);

		struct socket_context* sc = create_socket_context(client_socket,
				buffer);

		if (sc == NULL)
		{
			fprintf(stderr, "Error creating socket context. Socket=%d\n",
					client_socket);
			return -1;
		}

		int result = add_input(sc);
		if (result == -1)
		{
			fprintf(stderr, "Error adding input data to storage. Socket=%d\n",
					client_socket);
			return -1;
		}
	}
	return nbytes;
}

int set_socket_write_mode(int client_socket)
{
	struct epoll_event ev;

	ev.events = EPOLLOUT;
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
	dprint("[write_response] socket=%d\n", client_socket);

	struct socket_context* sc = get_output(client_socket);

	int result = -1;
	if (sc != NULL && sc->response != NULL)
	{
		result = send(client_socket, sc->response, sc->response_len, 0);
		dprint("[write_response] [%s]\n", sc->response);
		dprint("Sent %d bytes as response.\n", result);

		if (result > 0 && sc->close_after_response == 1)
		{
			result = -1;
		}
	}
	else
	{
		fprintf(stderr, "Response is not ready for socket %d\n", client_socket);
	}

	destroy_socket_context(sc);
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
		fprintf(stderr, "Error creating socket: %m\n");
		return -1;
	}

	if (fcntl(handle, F_SETFL, O_NONBLOCK))
	{
		fprintf(stderr, "Could not make the socket non-blocking: %m\n");
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
		fprintf(stderr, "Error binding socket: %m\n");
		return -1;
	}
	dprint("Bound socket %d to address 'INADDR_ANY' and port %u\n", handle,
			port);

	if (listen(handle, SOMAXCONN))
	{
		fprintf(stderr, "Could not start listening on server socket %d: %m\n",
				handle);
		close_handle(handle);
		return -1;
	}
	dprint(
			"Server socket %d started listening to address 'INADDR_ANY' and port %u\n",
			handle, port);

	return handle;
}

// todo Change from handle array to structure with ssl marker
int process_incoming_connections(int server_sockets[])
{
	int pollsize = 1;

	struct epoll_event ev;
	struct epoll_event epoll_events[EPOLL_ARRAY_SIZE];

	if ((epoll_fd = epoll_create(pollsize)) < 0)
	{
		fprintf(stderr, "Could not create the epoll of file descriptors: %m\n");
		close_handle(server_sockets[0]);
		return 1;
	}

	ev.events = EPOLLIN;
	ev.data.u64 = 0LL;
	ev.data.fd = server_sockets[0];

	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_sockets[0], &ev) < 0)
	{
		fprintf(stderr, "Couldn't add server socket %d to epoll set: %m\n",
				server_sockets[0]);
		close_handle(server_sockets[0]);
		return -1;
	}

	while (1)
	{
		int result;
		dprint("Starting epoll_wait on %d file descriptors\n", pollsize);

		while ((result = epoll_wait(epoll_fd, epoll_events, EPOLL_ARRAY_SIZE,
				-1)) < 0)
		{
			if ((result < 0) && (errno != EINTR))
			{
				fprintf(stderr, "EPoll on %d file descriptors failed: %m\n",
						pollsize);
				close_handle(server_sockets[0]);
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
				if (handle == server_sockets[0])
				{
					fprintf(stderr, "EPoll on %d file descriptors failed: %m\n",
							pollsize);
					close_handle(server_sockets[0]);
					return -1;
				}
				else
				{
					dprint("Closing socket with handle %d\n", handle);
					close_handle(handle);
					continue;
				}
			}
			else if (events & EPOLLIN)
			{
				if (handle == server_sockets[0])
				{
					int client_socket;
					struct sockaddr_in clientaddr;
					socklen_t clientaddr_size = sizeof(clientaddr);
					char buffer[1024];

					while ((client_socket = accept(server_sockets[0],
							(struct sockaddr *) &clientaddr, &clientaddr_size))
							< 0)
					{
						if ((client_socket < 0) && (errno != EINTR))
						{
							fprintf(stderr, "Accept on socket %d failed: %m\n",
									server_sockets[0]);
							close_handle(server_sockets[0]);
							return -1;
						}
					}

					if (inet_ntop(AF_INET, &clientaddr.sin_addr.s_addr, buffer,
							sizeof(buffer)) != NULL)
					{
						dprint(
								"[Web Server] Accepted connection from %s:%u, assigned new sd %d\n",
								buffer, ntohs(clientaddr.sin_port),
								client_socket);
					}
					else
					{
						fprintf(stderr,
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
						close_handle(server_sockets[0]);
						return -1;
					}

					pollsize++;
				}
				else
				{
					if (read_incoming_data(handle) <= 0)
					{
						pollsize--;
						close_handle(handle);
						continue;
					}
				}
			}
			else if (events & EPOLLOUT)
			{
				if (handle != server_sockets[0])
				{
					if (write_response(handle) <= 0)
					{
						pollsize--;
						close_handle(handle);
						continue;
					}
					else
					{
						ev.events = EPOLLIN;
						ev.data.u64 = 0LL;
						ev.data.fd = handle;

						if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, handle, &ev) < 0)
						{
							printf(
									"Couldn't modify client socket %d in epoll set: %m\n",
									handle);
							close_handle(server_sockets[0]);
						}
					}
				}
			}
		}
	}

	close_handle(server_sockets[0]);

	return 0;
}
