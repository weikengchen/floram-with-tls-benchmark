#ifndef UINT128_H
#define UINT128_H

#include <inttypes.h>

typedef struct uint128_t uint128_t;

uint128_t * uint128_current_timestamp();

uint128_t * uint128_new();
uint128_t * uint128_from(uint64_t);
uint128_t * uint128_add(uint128_t *, uint128_t *);
uint128_t * uint128_subtract(uint128_t *, uint128_t *);
uint128_t * uint128_multiply(uint128_t *, uint128_t *);
uint128_t * uint128_divide(uint128_t *, uint128_t *);

int print_uint128(FILE * io, uint128_t * u128);

#endif