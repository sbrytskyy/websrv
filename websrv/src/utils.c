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

int get_current_dir(char* full_path, char* filename)
{
	if (getcwd(full_path, PATH_MAX) != NULL)
	{
		dprint("Current working dir: %s\n", full_path);
		strcat(full_path, filename);
		dprint("Full pathname: %s\n", full_path);
	}
	else
		perror("getcwd() error");
	return 0;
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
