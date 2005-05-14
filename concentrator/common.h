#ifndef TOOLS_H
#define TOOLS_H

#include <stdio.h>
#include <stdint.h>
#include "ipfix.h"

#define debugf(message, ...) fprintf(stderr, "[DEBUG] %s l.%d: " message "\n", \
	__FILE__, __LINE__, __VA_ARGS__)

#define infof(message, ...) fprintf(stderr, "[INFO] %s l.%d: " message "\n", \
	__FILE__, __LINE__, __VA_ARGS__)

#define errorf(message, ...) fprintf(stderr, "[ERROR] %s l.%d: " message "\n", \
	__FILE__, __LINE__, __VA_ARGS__)

#define fatalf(message, ...) fprintf(stderr, "[FATAL] %s l.%d: " message "\n", \
	__FILE__, __LINE__, __VA_ARGS__)

#define debug(message) fprintf(stderr, "[DEBUG] %s l.%d: " message "\n", \
	__FILE__, __LINE__)

#define info(message) fprintf(stderr, "[INFO] %s l.%d: " message "\n", \
	__FILE__, __LINE__)

#define error(message) fprintf(stderr, "[ERROR] %s l.%d: " message "\n", \
	__FILE__, __LINE__)

#define fatal(message) fprintf(stderr, "[FATAL] %s l.%d: " message "\n", \
	__FILE__, __LINE__)

uint64_t ntohll(uint64_t i);
uint64_t htonll(uint64_t i);

#endif
