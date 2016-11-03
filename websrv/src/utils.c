/*
 * utils.c
 *
 *  Created on: May 16, 2016
 *      Author: sbrytskyy
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "utils.h"

char root_dir[PATH_MAX];

const char* get_root_dir()
{
	if (root_dir[0] == '\0')
	{
		if (getcwd(root_dir, PATH_MAX) == NULL)
		{
			perror("getcwd() error");
			return NULL;
		}
		if (strcat(root_dir, "/root") == NULL)
		{
			fprintf(stderr, "strcat error: %m\n");
			return NULL;
		}

	}
	return root_dir;
}

int startsWith(const char *pre, const char *str)
{
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? 0 : strncmp(pre, str, lenpre) == 0;
}
