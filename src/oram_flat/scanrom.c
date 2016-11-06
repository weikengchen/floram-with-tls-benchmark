#include "scanrom.h"
#include "flatoram_util.h"

void scanrom_create_local_halfpad(void * dest, void * key, size_t size) {
	offline_expand(dest, key, size);
}

void scanrom_read_with_bitvector_offline(void* data, void* local_data, bool * bitvector, size_t fullblocksize, size_t blockcount) {
	memset(data, 0, fullblocksize);

	for (size_t jj = 0; jj < fullblocksize /sizeof(uint64_t); jj++) {
		uint64_t sum = 0;
		#pragma omp parallel for reduction (^:sum)
		for (size_t ii = 0; ii < blockcount; ii++) {
			if (bitvector[ii]) {
				sum ^= ((uint64_t *)local_data)[ii * ((fullblocksize) /sizeof(uint64_t)) + jj];
			}
		}
		((uint64_t *)data)[jj] = sum;
	}
}

void scanrom_encrypt_offline(void * out, void * in, void* pad, size_t len) {
	#pragma omp parallel for
	for (size_t ii = 0; ii < len / sizeof(uint64_t); ii++) {
		((uint64_t *)out)[ii] = ((uint64_t *)in)[ii] ^ ((uint64_t *)pad)[ii];
	}	
}