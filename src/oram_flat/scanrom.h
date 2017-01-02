#ifndef SCANROM_H
#define SCANROM_H
#include "flatoram.h"

void scanrom_create_local_halfpad(void * dest, void * key, size_t size);

void scanrom_read_with_bitvector_offline(void* data, void* local_data, bool * bitvector, size_t fullblocksize, size_t blockcount);

void scanrom_encrypt_offline(void * out, void * in, void* key, size_t len);

void scanwrom_write_with_blockvector_offline(uint8_t * local_data, uint8_t * blockvector, bool * bitvector, uint8_t*Zblock, bool expand, size_t fullblocksize, size_t blockcount);

#endif