/*
 * data.h
 *
 *  Created on: May 3, 2016
 *      Author: sbrytskyy
 */

#ifndef DATA_H_
#define DATA_H_

struct SocketData
{
	int client_socket;
	char* pCharData;
	fd_set* write_fd_set;
};

#endif /* DATA_H_ */
