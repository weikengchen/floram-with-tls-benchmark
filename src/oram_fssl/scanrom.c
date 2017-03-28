#include "scanrom.h"
#include "floram_util.h"
#include <wmmintrin.h>
#include <tmmintrin.h>

void scanrom_read_with_bitvector_offline(uint8_t * data, uint8_t * local_data, bool * bitvector, size_t memblocksize, size_t blockcount) {
	memset(data, 0, memblocksize);

	uint64_t * d = local_data;
	bool * b = bitvector;

	for (size_t jj = 0; jj < memblocksize /sizeof(uint64_t); jj++) {
		uint64_t sum = 0;
		#pragma omp parallel for simd aligned(d,b:16) reduction(^:sum)
		for (size_t ii = 0; ii < blockcount; ii++) {
			if (b[ii]) {
				sum ^= d[ii * ((memblocksize) /sizeof(uint64_t)) + jj];
			}
		}
		((uint64_t *)data)[jj] = sum;
	}
}

#ifdef SCANROM_DISABLE_ENCRYPTION
void scanrom_encrypt_offline(uint8_t * out, uint8_t * in, uint8_t* key, size_t index, size_t blockmultiple, size_t blockcount) {
	if (in == NULL) {
		memset(out, 0, BLOCKSIZE * blockcount * blockmultiple);
	} else {
		memcpy(out, in, BLOCKSIZE * blockcount * blockmultiple);
	}
}
#else
void scanrom_encrypt_offline(uint8_t * out, uint8_t * in, uint8_t* key, size_t index, size_t blockmultiple, size_t blockcount) {
	//TODO multithread:
	offline_expand_from(out, key, index*blockmultiple, blockcount * blockmultiple);
	if (in != NULL) {
		#pragma omp parallel for simd
		for (size_t ii = 0; ii < blockcount * blockmultiple * BLOCKSIZE / sizeof(uint64_t); ii++) {
			((uint64_t *)out)[ii] ^= ((uint64_t *)in)[ii];
		}
	}
}
#endif

void scanwrom_write_with_blockvector_offline(uint8_t * local_data, uint8_t * blockvector, bool * bitvector, uint8_t*Zblock, size_t memblocksize, size_t blockcount) {
	uint64_t * d = local_data;
	uint64_t * b = blockvector;
	uint64_t * z = Zblock;

	#pragma omp parallel for
	for (size_t ii = 0; ii< blockcount; ii++) {
		if (bitvector[ii]) {
			#pragma omp simd aligned(d,b,z:16)
			for (size_t jj = 0; jj < memblocksize/sizeof(uint64_t); jj++) {
				d[ii * memblocksize/sizeof(uint64_t) + jj] ^= b[ii * memblocksize/sizeof(uint64_t) + jj] ^ z[jj];
			}
		} else {
			#pragma omp simd aligned(d,b:16)
			for (size_t jj = 0; jj < memblocksize/sizeof(uint64_t); jj++) {
				d[ii * memblocksize/sizeof(uint64_t) + jj] ^= b[ii * memblocksize/sizeof(uint64_t) + jj];
			}
		}
	}
}