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

int process_http(struct socket_context* pSc)
{
	pSc->close_after_response = 1;
	char* index = strstr(pSc->pRequest, "GET");
	if (index == pSc->pRequest)
	{
		char* response =
				"HTTP/1.1 200 OK\r\n\r\n<html>\r\n<body>\r\n<h1>Hello, World!</h1>\r\n</body>\r\n</html>";

		pSc->pResponse = malloc(strlen(response) + 1);
		if (pSc->pResponse == NULL)
		{
			fprintf(stderr, "Error creating response.\n");
			return -1;
		}
		strcpy(pSc->pResponse, response);
	}
	else
	{
		fprintf(stderr, "Unknown protocol!\n");
	}

	return 0;
}
