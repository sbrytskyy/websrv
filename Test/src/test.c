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
	init_storage();

	for (int i = 0; i < 100; i++)
	{
		char str[16];
		sprintf(str, "%d", i);
		struct socket_context* sc = create_socket_context(i, str);
		printf("%s\n", str);

		add_input(sc);
	}

	struct socket_context* sc;

	while ((sc = poll_first_input()) != NULL)
	{
		printf("Request: [%s]\n", sc->request);
	}

	cleanup_storage();
	return 0;
}

