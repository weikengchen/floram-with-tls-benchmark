#ifndef SCANROM_H
#define SCANROM_H
#include "floram.h"

typedef void (* duplexer_fn)(void *, void *);

void scanrom_read_with_bitvector_offline(uint8_t * output_share, uint8_t * rom_memory, bool * bitvector, size_t memblocksize, size_t blockcount);
void scanrom_read_with_blockvector_offline(uint8_t * z_and_output, uint8_t * rom_memory, bool * bitvector, uint8_t * blockvector, size_t memblocksize, size_t blockcount);

void scanrom_encrypt_offline(uint8_t * out, uint8_t * in, uint8_t* key, size_t index, size_t blockmultiple, size_t blockcount);

void scanwrom_write_with_blockvector_offline(uint8_t * wrom_memory, uint8_t * blockvector, bool * bitvector, uint8_t*z_block, size_t memblocksize, size_t blockcount);

#endif