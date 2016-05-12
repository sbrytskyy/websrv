/*
 * test.c
 *
 *  Created on: May 12, 2016
 *      Author: sbrytskyy
 */

#include <stdio.h>

#include "storage.h"

int test_storage()
{
	init_context_storage();

	for (int i = 0; i < 100; i++)
	{
		char str[16];
		sprintf(str, "%d", i);
		struct SocketContext* pSc = create_socket_context(i, str);
		printf("%s\n", str);

		add_input(pSc);
	}

	struct SocketContext* pSc;

	while ((pSc = get_first_input()) != NULL)
	{
		printf("Request: [%s]\n", pSc->pRequest);
	}

	destroy_context_storage();
	return 0;
}

