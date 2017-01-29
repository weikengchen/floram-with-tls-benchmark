#ifndef BITPROPAGATE_CPRG_H
#define BITPROPAGATE_CPRG_H
#include "floram.h"
#include "bitpropagate.h"

typedef struct bitpropagator_cprg_offline bitpropagator_cprg_offline;

typedef void (* bp_cprg_traverser_fn)(void*, void *, void *, void*, void*);

void bitpropagator_cprg_offline_start(uint8_t * local_output, bool * local_bit_output, uint64_t * accumulator_L, uint64_t * accumulator_R, bitpropagator_cprg_offline * bpo);
void bitpropagator_cprg_offline_process_round(uint8_t * accumulator_L, uint8_t * accumulator_R, uint8_t * z, bool advicebit_l, bool advicebit_r, bitpropagator_cprg_offline * bpo);
void bitpropagator_cprg_offline_finalize(uint8_t * accumulator, uint8_t * z, bool advicebit_l, bool advicebit_r, bitpropagator_cprg_offline * bpo);

void bitpropagator_cprg_offline_parallelizer(void* bp, void* indexp, void* blockdelta, void * local_output, void * local_bit_output, void* pd, bp_cprg_traverser_fn fn, facb_fn cbfn, void* cbpass);

bitpropagator_cprg_offline * bitpropagator_cprg_offline_new(size_t size, uint8_t * keyL, uint8_t * keyR);
void bitpropagator_cprg_offline_free(bitpropagator_cprg_offline * bpo);

#endif