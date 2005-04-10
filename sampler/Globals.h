/*
 * PSAMP Reference Implementation
 *
 * Globals.h
 *
 * global definitions used almost everywhere
 *
 * Author: Michael Drueing <michael@drueing.de>
 *
 */

#ifndef GLOBALS_H
#define GLOBALS_H

#ifdef DEBUG
#include <cstdio>
#define LOG(fmt, ...)      fprintf(stderr, fmt, ## __VA_ARGS__);
#else
#define LOG(fmt, ...)      do { } while (0);
#endif

#endif
