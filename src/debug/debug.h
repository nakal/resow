
#ifndef RESOW_DEBUG_INCLUDED
#define RESOW_DEBUG_INCLUDED

#include <assert.h>

#ifdef DEBUG

#include <stdio.h>

#define DEBUG_PRINT(msg) \
	fprintf(stderr, "%s at %s:%u\n", msg, __FILE__, __LINE__)

#else

#define DEBUG_PRINT(msg)

#endif

#endif

