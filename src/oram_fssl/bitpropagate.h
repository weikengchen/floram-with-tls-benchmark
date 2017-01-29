#ifndef BITPROPAGATE_H
#define BITPROPAGATE_H
#include "floram.h"

typedef struct bitpropagator_offline bitpropagator_offline;

typedef void (* bp_traverser_fn)(void *, void *);
typedef void (* bp_pusher_fn)(void *, void *, void *);
typedef void (* facb_fn)(void *, void*);

void bitpropagator_offline_start(bitpropagator_offline * bpo, void * blocks);
void bitpropagator_offline_push_Z(bitpropagator_offline * bpo, void * Z, bool advicebit_l, bool advicebit_r, size_t level);
void bitpropagator_offline_readblockvector(void * local_output, void* local_bit_output, bitpropagator_offline * bpo);

void bitpropagator_offline_parallelizer(void* bp, bitpropagator_offline * bpo, void* indexp, void * local_output, void* local_bit_output, void* pd, bp_traverser_fn fn, bp_pusher_fn fn2, facb_fn cbfn, void* cbpass);

bitpropagator_offline * bitpropagator_offline_new(size_t size, size_t startlevel, uint8_t * keyL, uint8_t * keyR);
void bitpropagator_offline_free(bitpropagator_offline * bpo);

#endif