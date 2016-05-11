/*
 * protocol.c
 *
 *  Created on: May 11, 2016
 *      Author: sbrytskyy
 */

#include <string.h>
#include <stdlib.h>

#include "protocol.h"

int process_http(struct SocketContext* pSc)
{
	pSc->close_after_response = 1;
	char* index = strstr(pSc->pRequest, "GET");
	if (index == pSc->pRequest)
	{
//		char* response =
//				"HTTP/1.1 200 OK\r\n<html>\r\n<body>\r\n<h1>Hello, World!</h1>\r\n</body>\r\n</html>";
		char* response = "Response\n";

		pSc->pResponse = malloc(strlen(response) + sizeof(char));
		strcpy(pSc->pResponse, response);
	}

	return 0;
}
