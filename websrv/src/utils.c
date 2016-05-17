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

char* read_file(char* filename)
{
	char* buffer = 0;
	long length;
	FILE* f = fopen(filename, "rb");

	if (f)
	{
		fseek(f, 0, SEEK_END);
		length = ftell(f);
		fseek(f, 0, SEEK_SET);
		buffer = malloc(length);
		if (buffer)
		{
			size_t res = fread(buffer, 1, length, f);
			if (res != length)
			{
				fprintf(stderr, "Error reading file: %s\n", filename);
			}
		}
		fclose(f);
	}

	return buffer;
}
