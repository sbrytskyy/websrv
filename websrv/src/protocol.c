/*
 * protocol.c
 *
 *  Created on: May 11, 2016
 *      Author: sbrytskyy
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

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

		// todo optimize twice allocation
		char* response = read_file(full_path);
		if (response)
		{
			pSc->pResponse = malloc(
					strlen(response_header) + strlen(response) + 1);
			if (pSc->pResponse != NULL)
			{
				if (strcpy(pSc->pResponse, response_header) == NULL
						|| strcat(pSc->pResponse, response) == NULL)
				{
					fprintf(stderr, "Error creating response.\n");
					free(pSc->pResponse);
				}
			}
			free(response);
		}
	}
	else
	{
		fprintf(stderr, "Unknown protocol!\n");
	}

	return 0;
}
