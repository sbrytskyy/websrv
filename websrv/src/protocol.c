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

static const char* INDEX_HTML = "index.html";
static const char* METHOD_GET = "GET";
static const char* RESPONSE_HEADER_200_OK = "HTTP/1.1 200 OK\r\n\r\n";

int process_http(struct socket_context* pSc)
{
	pSc->close_after_response = 1;
	char* index = strstr(pSc->pRequest, METHOD_GET);
	if (index == pSc->pRequest)
	{

		// todo think how to optimize 0. keep one buffer per worker 1. by copying once

		char full_path[PATH_MAX];
		full_path[0] = '\0';

		if (strcpy(full_path, get_root_dir()) == NULL)
		{
			fprintf(stderr, "strcpy error: %m\n");
			return -1;
		}

		char *token = NULL;
		char* request_uri = NULL;
		token = strtok(pSc->pRequest, "\r\n");
		if (token)
		{
			char* method = strtok(token, " ");
			dprint("Method: %s\n", method);
			if (method && strcmp(METHOD_GET, method) == 0)
			{
				request_uri = strtok(NULL, " ");
				dprint("Request-URI: %s\n", request_uri);
				if (request_uri)
				{
					char* http_version = strtok(NULL, " ");
					dprint("HTTP-Version: %s\n", http_version);
				}
			}
		}

		if (request_uri == NULL)
		{
			fprintf(stderr, "Error parsing request.\n");
			return -1;
		}

		if (strcat(full_path, request_uri) == NULL)
		{
			fprintf(stderr, "strcat error: %m\n");
			return -1;
		}
		if (strlen(request_uri) == 1 && request_uri[0] == '/')
		{

			if (strcat(full_path, INDEX_HTML) == NULL)
			{
				fprintf(stderr, "strcat error: %m\n");
				return -1;
			}
		}

		int fd = open(full_path, O_RDONLY);
		if (fd == -1)
		{
			fprintf(stderr, "Error opening file: %s; error - %m\n", full_path);
			return -1;
		}

		int len = lseek(fd, 0, SEEK_END);
		char* data = mmap(0, len, PROT_READ, MAP_PRIVATE, fd, 0);

		if (data)
		{
			pSc->pResponse = malloc(strlen(RESPONSE_HEADER_200_OK) + len + 1);
			if (pSc->pResponse != NULL)
			{
				if (strcpy(pSc->pResponse, RESPONSE_HEADER_200_OK) == NULL
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
	else
	{
		fprintf(stderr, "Unknown protocol!\n");
	}

	return 0;
}
