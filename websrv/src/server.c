#include "server.h"


int read_incoming_data(int client_socket, fd_set* write_fd_set)
{
	char buffer[MAX_PACKET_SIZE];

	int nbytes = read(client_socket, buffer, 512);
	if (nbytes < 0)
	{
		perror("Error reading socket");
		return -1;
	}
	else if (nbytes == 0)
	{
		return -1;
	}
	else
	{
		int index = nbytes;
		if (nbytes > 2 && buffer[nbytes-2] == '\r' && buffer[nbytes-1] == '\n')
		{
			index = nbytes - 2;
		}
		buffer[index] = '\0';
		printf("Received message [%s], length: %d\n", buffer, nbytes);

		struct SocketContext* pSc = malloc(sizeof(struct SocketContext));
		pSc->client_socket = client_socket;
		pSc->pRequest = malloc(strlen(buffer) + sizeof(char));
		strcpy(pSc->pRequest, buffer);
		pSc->write_fd_set = write_fd_set;

		store_socket_context(pSc);

		// todo refactor worker
		start_worker(pSc);

		return 0;
	}
}

int write_response(int client_socket)
{
	//char* response = "HTTP/1.1 200 OK\n<html>\n<body>\n<h1>Hello, World!</h1>\n</body>\n</html>";

	printf("[write_response] socket=%d\n", client_socket);

	struct SocketContext* pSc = get_socket_context(client_socket);

	char* response = pSc->pResponse;
	printf("[write_response] [%s]\n", response);

	int result = send(client_socket, response, strlen(response), 0);

	remove_socket_context(pSc);

	return result;
}

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
	init_context_storage();

	fd_set active_fd_set, read_fd_set, write_fd_set;
	struct sockaddr_in clientaddr;
	unsigned int clientaddr_size = sizeof(clientaddr);

	FD_ZERO(&active_fd_set);
	FD_ZERO(&write_fd_set);
	FD_SET(server_socket, &active_fd_set);

	while(1)
	{
		read_fd_set = active_fd_set;
		printf("Select running...");
		if (select(FD_SETSIZE, &read_fd_set, &write_fd_set, NULL, NULL) < 0)
		{
			perror("Select function failed.");
			return -1;
		}

		printf("Select fired...");
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

					printf ("[Web Server] connect from host %s, port %hd.\n", inet_ntoa (clientaddr.sin_addr), ntohs (clientaddr.sin_port));

					FD_SET(client_socket, &active_fd_set);
				}
				else
				{
					if (read_incoming_data(i, &write_fd_set) < 0)
					{
						printf("End of connection to %d\n", i);
						close(i);
						FD_CLR(i, &active_fd_set);
					}
				}
			}
			else if (FD_ISSET(i, &write_fd_set))
			{
				int nsent = write_response(i);
				printf("Sent %d bytes as response.\n", nsent);
				close(i);
				FD_CLR(i, &write_fd_set);
				FD_CLR(i, &active_fd_set);
			}
		}
	}

	return 0;
}
