/*
 * protocol.h
 *
 *  Created on: May 11, 2016
 *      Author: sbrytskyy
 */

#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#include "storage.h"

int process_http(struct socket_context* sc, char* full_path);

#endif /* PROTOCOL_H_ */
