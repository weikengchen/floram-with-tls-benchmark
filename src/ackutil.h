#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <obliv_common.h>

//Note: the following macros CANNOT be used with obliv arguments (in case it wasn't obivious).

#define LOG2LL(X) ((unsigned) (8*sizeof (unsigned long long) - __builtin_clzll((X)) - 1))

#define MAX(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define MIN(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

uint64_t current_timestamp();
uint32_t rand_range(uint32_t, uint32_t);

#endif

