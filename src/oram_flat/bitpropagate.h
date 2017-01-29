#ifndef BITPROPAGATE_H
#define BITPROPAGATE_H
#include "flatoram.h"

typedef struct bitpropagator_offline bitpropagator_offline;

typedef void (* bp_traverser_fn)(void*, void *, void *, void*, void*);
typedef void (* facb_fn)(void *, void*);

void bitpropagator_offline_start(uint8_t * local_output, bool * local_bit_output, uint64_t * accumulator_L, uint64_t * accumulator_R, bitpropagator_offline * bpo);
void bitpropagator_offline_process_round(uint8_t * accumulator_L, uint8_t * accumulator_R, uint8_t * z, bool advicebit_l, bool advicebit_r, bitpropagator_offline * bpo);
void bitpropagator_offline_finalize(uint8_t * accumulator, uint8_t * z, bool advicebit_l, bool advicebit_r, bitpropagator_offline * bpo);

void bitpropagator_offline_parallelizer(void* bp, void* indexp, void* blockdelta, void * local_output, void * local_bit_output, void* pd, bp_traverser_fn fn, facb_fn cbfn, void* cbpass);

bitpropagator_offline * bitpropagator_offline_new(size_t size, uint8_t * keyL, uint8_t * keyR);
void bitpropagator_offline_free(bitpropagator_offline * bpo);

#endif