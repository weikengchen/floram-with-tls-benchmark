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


void scanwrom_write_with_blockvector_offline(uint8_t * local_data, uint8_t * blockvector, bool * controlbitvector, uint8_t*Zblock, bool expand, size_t fullblocksize, size_t blockcount) {
	#pragma omp parallel for
	for (size_t ii = 0; ii< blockcount; ii++) {
		if (controlbitvector[ii]) {
			#pragma omp simd
			for (size_t jj = 0; jj < fullblocksize; jj++) {
				local_data[ii * fullblocksize + jj] ^= blockvector[ii * fullblocksize + jj] ^ Zblock[jj];
			}
		} else {
			#pragma omp simd
			for (size_t jj = 0; jj < fullblocksize; jj++) {
				local_data[ii * fullblocksize + jj] ^= blockvector[ii * fullblocksize + jj];
			}
		}
	}
}

void scanrom_transfer_duplexer(duplexer_fn fn1, duplexer_fn fn2, void* data, void * pd) {

	#pragma omp parallel num_threads(2)
	{
		#pragma omp master
		fn1(data, NULL);

		#pragma omp single
		{
			#pragma omp task
			fn2(data, pd);
		}
	}

}