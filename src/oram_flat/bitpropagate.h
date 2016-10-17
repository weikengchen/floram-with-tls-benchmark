#ifndef BITPROPAGATE_H
#define BITPROPAGATE_H
#include "flatoram.h"

typedef struct bitpropagator_offline bitpropagator_offline;

void bitpropagator_offline_start(bitpropagator_offline * bpo, void * blocks);
void bitpropagator_offline_nextlevel(bitpropagator_offline * bpo, void * Z, uint32_t advicebit);
void bitpropagator_offline_lastlevel(bitpropagator_offline * bpo, void * Z, uint32_t advicebit);
void bitpropagator_offline_readblockvector(void * local_output, bitpropagator_offline * bpo);

bitpropagator_offline * bitpropagator_offline_new(size_t size, uint32_t startlevel);
void bitpropagator_offline_free(bitpropagator_offline * bpo);

#endif