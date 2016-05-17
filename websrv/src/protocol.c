/*
 * protocol.c
 *
 *  Created on: May 11, 2016
 *      Author: sbrytskyy
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "protocol.h"
#include "utils.h"

int process_http(struct socket_context* pSc)
{
	pSc->close_after_response = 1;
	char* index = strstr(pSc->pRequest, "GET");
	if (index == pSc->pRequest)
	{
		char* response_header = "HTTP/1.1 200 OK\r\n\r\n";
		char full_path[PATH_MAX];
		full_path[0] = '\0';

		if (strcpy(full_path, get_root_dir()) == NULL)
		{
			fprintf(stderr, "strcpy error: %m\n");
			return -1;
		}

		// todo hardcoded filename
		if (strcat(full_path, "/index.html") == NULL)
		{
			fprintf(stderr, "strcat error: %m\n");
			return -1;
		}

		int fd = open(full_path, O_RDONLY);
		if (fd)
		{
			int len = lseek(fd, 0, SEEK_END);
			char* data = mmap(0, len, PROT_READ, MAP_PRIVATE, fd, 0);

			if (data)
			{
				pSc->pResponse = malloc(
						strlen(response_header) + len + 1);
				if (pSc->pResponse != NULL)
				{
					if (strcpy(pSc->pResponse, response_header) == NULL
							|| strcat(pSc->pResponse, data) == NULL)
					{
						fprintf(stderr, "Error creating response.\n");
						free(pSc->pResponse);
					}
				}

				munmap(data, len);
			}
			close(fd);
		}
	}
	else
	{
		fprintf(stderr, "Unknown protocol!\n");
	}

	return 0;
}
