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
int floram_zpma(void** dst, size_t alignment, size_t size);

uint32_t floram_atomic_read(uint32_t *);
void floram_atomic_inc(uint32_t *);

int floram_usleep(uint64_t x);

void offline_expand(uint8_t * dest, uint8_t * src, size_t n);
void offline_expand_from(uint8_t * dest, uint8_t * src, size_t i, size_t n);

void offline_prg_init();
void * offline_prg_keyschedule(uint8_t * src);
void offline_prg(uint8_t * dest, uint8_t * src, void * r);
void offline_prg_oct(uint8_t * dest1, uint8_t * dest2, uint8_t * dest3, uint8_t * dest4,
						uint8_t * dest5, uint8_t * dest6, uint8_t * dest7, uint8_t * dest8, 
						uint8_t * src1, uint8_t * src2, uint8_t * src3, uint8_t * src4,
						uint8_t * src5, uint8_t * src6, uint8_t * src7, uint8_t * src8,
						void * ri1, void * ri2 , void * ri3 , void * ri4,
						void * ri5, void * ri6 , void * ri7 , void * ri8
					);

#endif