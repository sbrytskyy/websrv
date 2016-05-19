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

static const char* CRLF = "\r\n";
static const char* INDEX_HTML = "index.html";
static const char* METHOD_GET = "GET";
static const char* HEADER_ACCEPT = "Accept:";
static const char* HEADER_CONTENT_TYPE = "Content-Type:";
static const char* HEADER_CONTENT_LENGTH = "Content-Length:";
static const char* HEADER_CONNECTION_KEEP_ALIVE = "Connection: keep-alive";

static const char* DEFAULT_CONTENT_TYPE = "text/html";

static const char* RESPONSE_HEADER_200_OK = "HTTP/1.1 200 OK";

struct http_context
{
	const char* method;
	const char* request_uri;
	const char* http_version;
	const char* content_type;
	const char* content_length;
};

int process_header(char* request, struct http_context* hc)
{
	int result = -1;
	char *token = NULL;
	char *end_str;
	token = strtok_r(request, CRLF, &end_str);
	int line = 0;

	while (token)
	{
		//dprint("token: %s\n", token);
		if (line == 0)
		{
			char *end_token;
			hc->method = strtok_r(token, " ", &end_token);
			dprint("Method: %s\n", hc->method);
			if (hc->method && strcmp(METHOD_GET, hc->method) == 0)
			{
				hc->request_uri = strtok_r(NULL, " ", &end_token);
				dprint("Request-URI: %s\n", hc->request_uri);
				if (hc->request_uri == NULL)
				{
					break;
				}

				hc->http_version = strtok_r(NULL, " ", &end_token);
				dprint("HTTP-Version: %s\n", hc->http_version);

				result = 0;
			}
		}

		token = strtok_r(NULL, CRLF, &end_str);
		if (token && strstr(token, HEADER_ACCEPT) == token)
		{
			char *end_token;
			strtok_r(token, " ", &end_token);
			char* accept = strtok_r(NULL, " ", &end_token);

			accept = strtok_r(accept, ";", &end_token);
			hc->content_type = strtok_r(accept, ",", &end_token);
			dprint("Content-Type: %s\n", hc->content_type);
		}

		line++;
	}

	return result;
}

int process_http(struct socket_context* sc, char* full_path)
{
	// todo process keep-alive and decide to close connection or not. Currently do not close
//	sc->close_after_response = 1;

	char* index = strstr(sc->request, METHOD_GET);
	if (index == sc->request)
	{
		struct http_context hc;
		if (process_header(sc->request, &hc) == -1)
		{
			fprintf(stderr, "Error parsing request.\n");
			return -1;
		}

		if (hc.content_type == NULL)
		{
			hc.content_type = DEFAULT_CONTENT_TYPE;
		}

		if (strcat(full_path, hc.request_uri) == NULL)
		{
			fprintf(stderr, "strcat error: %m\n");
			return -1;
		}
		if (strlen(hc.request_uri) == 1 && hc.request_uri[0] == '/')
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

		int data_len = lseek(fd, 0, SEEK_END);
		char* data = mmap(0, data_len, PROT_READ, MAP_PRIVATE, fd, 0);

		if (data)
		{
			char len_str[15];
			sprintf(len_str, "%d", data_len);

			int header_len = strlen(RESPONSE_HEADER_200_OK) + strlen(CRLF)
					+ strlen(HEADER_CONTENT_TYPE) + 1 + strlen(hc.content_type) + strlen(CRLF)
					+ strlen(HEADER_CONTENT_LENGTH) + 1 + strlen(len_str) + strlen(CRLF)
					+ strlen(HEADER_CONNECTION_KEEP_ALIVE) + strlen(CRLF)
					+ strlen(CRLF);

			sc->response = malloc(header_len + data_len);
			if (sc->response != NULL)
			{
				if (strcpy(sc->response, RESPONSE_HEADER_200_OK) == NULL
						|| strcat(sc->response, CRLF) == NULL
						|| strcat(sc->response, HEADER_CONTENT_TYPE) == NULL
						|| strcat(sc->response, " ") == NULL
						|| strcat(sc->response, hc.content_type) == NULL
						|| strcat(sc->response, CRLF) == NULL
						|| strcat(sc->response, HEADER_CONTENT_LENGTH) == NULL
						|| strcat(sc->response, " ") == NULL
						|| strcat(sc->response, len_str) == NULL
						|| strcat(sc->response, CRLF) == NULL
						|| strcat(sc->response, HEADER_CONNECTION_KEEP_ALIVE) == NULL
						|| strcat(sc->response, CRLF) == NULL
						|| strcat(sc->response, CRLF) == NULL)
				{
					fprintf(stderr, "Error creating response.\n");
					free(sc->response);
				}
				else
				{
					memcpy(sc->response + header_len, data, data_len);

					sc->response_len = header_len + data_len;
				}
			}

			munmap(data, data_len);
		}
		close(fd);
	}
	else
	{
		fprintf(stderr, "Unknown protocol!\n");
	}

	return 0;
}
