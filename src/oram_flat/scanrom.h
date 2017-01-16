#ifndef SCANROM_H
#define SCANROM_H
#include "flatoram.h"

typedef void (* duplexer_fn)(void *, void *);

void scanrom_transfer_duplexer(duplexer_fn fn1, duplexer_fn fn2, void* data, void * pd);

void scanrom_create_local_halfpad(uint8_t * dest, uint8_t * key, size_t size);

void scanrom_read_with_bitvector_offline(uint8_t * data, uint8_t * local_data, bool * bitvector, size_t fullblocksize, size_t blockcount);

void scanrom_encrypt_offline(uint8_t * out, uint8_t * in, uint8_t* pad, size_t len);

void scanwrom_write_with_blockvector_offline(uint8_t * local_data, uint8_t * blockvector, bool * bitvector, uint8_t*Zblock, bool expand, size_t fullblocksize, size_t blockcount);

#endif