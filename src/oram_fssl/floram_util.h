#ifndef floram_UTIL_H
#define floram_UTIL_H
#include "floram.h"

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
int floram_pma(void** dst, size_t alignment, size_t size);

typedef void (* generic_function)(void *);
void network_parallelizer(generic_function fn1, generic_function fn2, void* data1, void* data2);

void offline_expand_init();
void offline_expand_deinit();
void offline_expand(uint8_t * dest, uint8_t * src, size_t n);

void * offline_prf_keyschedule(uint8_t * src);
void offline_prf(uint8_t * dest, uint8_t * src, void * r);

#endif