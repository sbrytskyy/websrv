/*
 * utils.h
 *
 *  Created on: May 16, 2016
 *      Author: sbrytskyy
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <stdio.h>

#ifdef DEBUG
#define dprint(...) \
		do { fprintf(stdout, __VA_ARGS__); } while (0)
#else
#define dprint(...) \
		do {  } while (0)
#endif

#endif /* UTILS_H_ */
