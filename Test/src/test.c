/*
 * test.c
 *
 *  Created on: May 17, 2016
 *      Author: sbrytskyy
 */

#include <string.h>
#include <stdio.h>

char response[] =
		"GET / HTTP/1.1\r\nHost: localhost:8080\r\nUser-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:46.0) Gecko/20100101 Firefox/46.0\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";

int main() {
	char *token = NULL;
	token = strtok(response, "\r\n");
	if (token) {
		printf("First token: %s\n", token);
		token = strtok(token, " ");
		printf("Command: %s\n", token);
		token = strtok(NULL, " ");
		printf("URL: %s\n", token);
		token = strtok(NULL, " ");
		printf("Protocol: %s\n", token);
	}
	return 0;
}
