#ifndef BITPROPAGATE_H
#define BITPROPAGATE_H
#include "flatoram.h"

typedef struct bitpropagator_offline bitpropagator_offline;

typedef void (* bp_traverser_fn)(void *, void *);

void bitpropagator_offline_start(bitpropagator_offline * bpo, void * blocks);
void bitpropagator_offline_push_Z(bitpropagator_offline * bpo, void * Z, uint32_t advicebit, size_t level);
void bitpropagator_offline_readblockvector(void * local_output, bitpropagator_offline * bpo);

void bitpropagator_offline_parallelizer(void* bp, bitpropagator_offline * bpo, void* indexp, void * local_output, bp_traverser_fn fn);

bitpropagator_offline * bitpropagator_offline_new(size_t size, size_t startlevel);
void bitpropagator_offline_free(bitpropagator_offline * bpo);

#endif