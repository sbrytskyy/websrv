#include "server.h"

int init_server_socket (uint16_t port)
{
	int handle;
	struct sockaddr_in serveraddr;

	handle = socket(PF_INET, SOCK_STREAM, 0);
	if (handle < 0)
	{
		perror("Error creating socket");
		return -1;
	}

	memset(&serveraddr, 0, sizeof(serveraddr));

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(port);
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(handle, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
	{
		perror("Error binding socket");
		return -1;
	}

	return handle;
}

int process_incoming_connections(int server_socket)
{
	fd_set active_fd_set, read_fd_set;
	struct sockaddr_in clientaddr;
	unsigned int clientaddr_size = sizeof(clientaddr);

	FD_ZERO(&active_fd_set);
	FD_SET(server_socket, &active_fd_set);

	while(1)
	{
		read_fd_set = active_fd_set;
		if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0)
		{
			perror("Select function failed.");
			return -1;
		}

		for(int i = 0; i < FD_SETSIZE; ++i)
		{
			if (FD_ISSET(i, &read_fd_set))
			{
				if (i == server_socket)
				{
					int client_socket = accept(server_socket, (struct sockaddr *) &clientaddr, &clientaddr_size);
					if (client_socket < 0)
					{
						perror("Error accepting client connection");
						return -1;
					}

					printf ("Server: connect from host %s, port %hd.\n", inet_ntoa (clientaddr.sin_addr), ntohs (clientaddr.sin_port));

					FD_SET(client_socket, &active_fd_set);
				}
				else
				{
					// todo read data from socket

					close(i);
					FD_CLR(i, &active_fd_set);
				}
			}
		}
	}

	return 0;
}
