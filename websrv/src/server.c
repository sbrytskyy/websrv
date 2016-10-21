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

#define EPOLL_ARRAY_SIZE 64

#define CONNECTION_CACHE_SIZE 32

int epoll_fd;

SSL_CTX *ssl_ctx;
struct connection_info *connections;
struct connection_info *connections_cache;

// todo rework buffer handling
char buffer[MAX_PACKET_SIZE];

int read_incoming_data(struct connection_info *cs)
{
	int nbytes = -1;

	if (cs == NULL)
	{
		fprintf(stderr, "Read incoming data failed. Empty connection structure passed.\n");
		return -1;
	}

	if (cs->is_ssl)
	{
		// todo add ssl support
		dprint("Socket %d is secured. SSL will be added.\n", cs->handle);
		//nbytes = read(cs->handle, buffer, sizeof(buffer));
        uint8_t *read_buf;
        if ((nbytes = ssl_read(cs->ssl, &read_buf)) > SSL_OK)
        {
            memcpy(buffer, read_buf, nbytes > (int)sizeof(buffer) ? sizeof(buffer) : nbytes);
        }
	}
	else
	{
		dprint("Reading from unsecured Socket: %d.\n", cs->handle);
		nbytes = read(cs->handle, buffer, sizeof(buffer));
	}
	if (nbytes < 0)
	{
		fprintf(stderr, "Error reading socket %d: %m\n", cs->handle);
	}
	else if (nbytes == 0 && (cs->is_ssl == 0))
	{
		dprint("Socket %d has been closed by client: %m\n", cs->handle);
	}
	else
	{
		if (nbytes > 0)
		{
			buffer[nbytes] = '\0';
			dprint("Received message [%s], length: %d\n\n", buffer, nbytes);
			dprint("Received bytes: %d\n", nbytes);

			struct socket_context* sc = create_socket_context(cs->handle,
					buffer);

			if (sc == NULL)
			{
				fprintf(stderr, "Error creating socket context. Socket=%d\n",
						cs->handle);
				return -1;
			}

			int result = add_input(sc);
			if (result == -1)
			{
				fprintf(stderr, "Error adding input data to storage. Socket=%d\n",
						cs->handle);
				return -1;
			}
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

int write_response(struct connection_info *cs)
{
	if (cs == NULL)
	{
		fprintf(stderr, "Write response failed. Empty connection structure passed.\n");
		return -1;
	}

	dprint("[write_response] socket=%d\n", cs->handle);

	struct socket_context* sc = get_output(cs->handle);

	int result = -1;
	if (sc != NULL && sc->response != NULL)
	{
		if (cs->is_ssl)
		{
			//result = send(cs->handle, sc->response, sc->response_len, 0);

	        SSL *ssl = cs->ssl;
	        result = ssl ? ssl_write(ssl, (uint8_t *)sc->response, sc->response_len) : -1;

			dprint("[write_response] using secured SSL connection.\n");
			//dprint("[write_response] using SSL connection: [%s]\n", sc->response);
		}
		else
		{
			result = send(cs->handle, sc->response, sc->response_len, 0);
			dprint("[write_response] using unsecured plain socket.\n");
			//dprint("[write_response] using plain socket: [%s]\n", sc->response);
		}
		dprint("Sent %d bytes as response.\n", result);

		if (result > 0 && sc->close_after_response == 1)
		{
			result = -1;
		}
	}
	else
	{
		fprintf(stderr, "Response is not ready for socket %d\n", cs->handle);
	}

	destroy_socket_context(sc);
	return result;
}

void addconnection(int handle, int is_ssl)
{
	dprint("Opened connection on socket %d; %s.\n", handle, (is_ssl ? "SSL Secured" : "Unsecured"));

	struct connection_info *cn;

	if (connections_cache == NULL)
		cn = malloc(sizeof(struct connection_info));
    else
    {
        cn = connections_cache;
        connections_cache = cn->next;
    }

	cn->next = connections;
	cn->handle = handle;
	cn->is_ssl = is_ssl;

	if (is_ssl)
	{
		cn->ssl = ssl_server_new(ssl_ctx, handle);
	}

	connections = cn;
}

void close_handle(int handle)
{
	shutdown(handle, SHUT_RDWR);
	close(handle);
}

void removeconnection(struct connection_info *cn)
{
    if (cn == NULL)
    {
    	return;
    }

	struct connection_info *tp;
    int shouldret = 0;

    tp = connections;

    if (tp == NULL)
    {
        return;
    }

    if (tp == cn)
    	connections = tp->next;
    else
    {
        while (tp != NULL)
        {
            if (tp->next == cn)
            {
                tp->next = (tp->next)->next;
                shouldret = 0;
                break;
            }

            tp = tp->next;
            shouldret = 1;
        }
    }

    if (shouldret)
        return;

    cn->next = connections_cache;
    connections_cache = cn;

	if (cn->is_ssl)
	{
		ssl_free(cn->ssl);
		cn->ssl = NULL;
	}

	close_handle(cn->handle);
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

int init_connection_cache()
{
	struct connection_info *cn;
    for (int i = 0; i < CONNECTION_CACHE_SIZE; i++)
    {
    	cn = connections_cache;
        connections_cache = malloc(sizeof(struct connection_info));
        if (connections_cache == NULL)
        {
        	return -1;
        }
        connections_cache->next = cn;
    }

    return 0;
}

int process_incoming_connections(int server_socket, int secured_server_socket)
{
	if (init_connection_cache() < 0)
	{
		perror("Error init cache.");
		close_handle(server_socket);
		close_handle(secured_server_socket);
		return -1;
	}

	uint32_t options = CONFIG_HTTP_DEFAULT_SSL_OPTIONS;
	ssl_ctx = ssl_ctx_new(options, CONFIG_HTTP_SESSION_CACHE_SIZE);

	int pollsize = 1;

	struct epoll_event epoll_events[EPOLL_ARRAY_SIZE];

	if ((epoll_fd = epoll_create(pollsize)) < 0)
	{
		fprintf(stderr, "Could not create the epoll of file descriptors: %m\n");
		close_handle(server_socket);
		close_handle(secured_server_socket);
		return 11;
	}

	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.u64 = 0LL;
	ev.data.fd = server_socket;

	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &ev) < 0)
	{
		fprintf(stderr, "Couldn't add server socket %d to epoll set: %m\n",
				server_socket);
		close_handle(server_socket);
		close_handle(secured_server_socket);
		return -1;
	}

	ev.events = EPOLLIN;
	ev.data.u64 = 0LL;
	ev.data.fd = secured_server_socket;

	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, secured_server_socket, &ev) < 0)
	{
		fprintf(stderr, "Couldn't add secured server socket %d to epoll set: %m\n",
				secured_server_socket);
		close_handle(server_socket);
		close_handle(secured_server_socket);
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
				close_handle(server_socket);
				close_handle(secured_server_socket);
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
				if (handle == server_socket || handle == secured_server_socket)
				{
					fprintf(stderr, "EPoll on %d file descriptors failed: %m\n",
							pollsize);
					close_handle(server_socket);
					close_handle(secured_server_socket);
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
				if (handle == server_socket || handle == secured_server_socket)
				{
					int client_socket;
					struct sockaddr_in clientaddr;
					socklen_t clientaddr_size = sizeof(clientaddr);
					char buffer[1024];

					int is_ssl = (handle == secured_server_socket);

					while ((client_socket = accept(handle,
							(struct sockaddr *) &clientaddr, &clientaddr_size))
							< 0)
					{
						if ((client_socket < 0) && (errno != EINTR))
						{
							fprintf(stderr, "Accept on socket %d failed: %m\n",
									handle);
							close_handle(server_socket);
							close_handle(secured_server_socket);
							return -1;
						}
					}

					addconnection(client_socket, is_ssl);

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
						close_handle(server_socket);
						close_handle(secured_server_socket);
						return -1;
					}

					pollsize++;
				}
				else
				{
					struct connection_info *cs = connections;
					while (cs != NULL)
					{
						if (handle == cs->handle)
						{
							break;
						}
						cs = cs->next;
					}

					if (read_incoming_data(cs) <= 0)
					{
						if (cs->is_ssl == 0)
						{
							pollsize--;
							removeconnection(cs);
						}
						continue;
					}
				}
			}
			else if (events & EPOLLOUT)
			{
				if (handle != server_socket && handle != secured_server_socket)
				{
					struct connection_info *cs = connections;
					while (cs != NULL)
					{
						if (handle == cs->handle)
						{
							break;
						}
						cs = cs->next;
					}

					if (write_response(cs) <= 0)
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
							close_handle(server_socket);
							close_handle(secured_server_socket);
						}
					}
				}
			}
		}
	}

	ssl_ctx_free(ssl_ctx);
	close_handle(server_socket);
	close_handle(secured_server_socket);

	return 0;
}
