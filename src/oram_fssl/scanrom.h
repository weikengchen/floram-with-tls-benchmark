#ifndef SCANROM_H
#define SCANROM_H
#include "floram.h"

typedef void (* duplexer_fn)(void *, void *);

void scanrom_read_with_bitvector_offline(uint8_t * data, uint8_t * local_data, bool * bitvector, size_t fullblocksize, size_t blockcount);

void scanrom_encrypt_offline(uint8_t * out, uint8_t * in, uint8_t* key, size_t index, size_t blockmultiple, size_t blockcount);

void scanwrom_write_with_blockvector_offline(uint8_t * local_data, uint8_t * blockvector, bool * bitvector, uint8_t*Zblock, size_t memblocksize, size_t blockcount);

#endif