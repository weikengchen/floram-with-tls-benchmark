#ifndef SCANROM_H
#define SCANROM_H
#include "flatoram.h"

void scanrom_create_local_halfpad(void * dest, void * key, size_t size);

void scanrom_read_with_bitvector_offline(void* data, void* local_data, bool * bitvector, size_t fullblocksize, size_t blockcount);

void scanrom_encrypt_offline(void * out, void * in, void* key, size_t len);

#endif