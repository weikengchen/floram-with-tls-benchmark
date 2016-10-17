#ifndef FLATORAM_UTIL_H
#define FLATORAM_UTIL_H
#include "flatoram.h"

#define LOG2(X) ((unsigned) (8*sizeof (unsigned long long) - __builtin_clzll((X)) - 1))

//Note: the following macros CANNOT be used with obliv arguments (in case it wasn't obivious).

#define MAX(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define MIN(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

void get_random_bytes(void *buf, size_t bytes);

void offline_expand_init();
void offline_expand_deinit();
void offline_expand(void * dest, void * src, size_t n);

#endif