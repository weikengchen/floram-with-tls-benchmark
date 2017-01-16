#include "scanrom.h"
#include "flatoram_util.h"

void scanrom_create_local_halfpad(uint8_t * dest, uint8_t * key, size_t size) {
	offline_expand(dest, key, size);
}

void scanrom_read_with_bitvector_offline(uint8_t * data, uint8_t * local_data, bool * bitvector, size_t fullblocksize, size_t blockcount) {
	memset(data, 0, fullblocksize);

	uint64_t * d = local_data;
	bool * b = bitvector;

	for (size_t jj = 0; jj < fullblocksize /sizeof(uint64_t); jj++) {
		uint64_t sum = 0;
		#pragma omp parallel for simd aligned(d,b:16) reduction(^:sum)
		for (size_t ii = 0; ii < blockcount; ii++) {
			if (b[ii]) {
				sum ^= d[ii * ((fullblocksize) /sizeof(uint64_t)) + jj];
			}
		}
		((uint64_t *)data)[jj] = sum;
	}
}

void scanrom_encrypt_offline(uint8_t * out, uint8_t * in, uint8_t* pad, size_t len) {
	uint64_t * o = out;
	uint64_t * i = in;
	uint64_t * p = pad;

	#pragma omp parallel for simd aligned(o,i,p:16)
	for (size_t ii = 0; ii < len / sizeof(uint64_t); ii++) {
		o[ii] = i[ii] ^ p[ii];
	}	
}


void scanwrom_write_with_blockvector_offline(uint8_t * local_data, uint8_t * blockvector, bool * controlbitvector, uint8_t*Zblock, bool expand, size_t fullblocksize, size_t blockcount) {
	uint8_t * d = local_data;
	uint8_t * b = blockvector;
	uint8_t * z = Zblock;

	#pragma omp parallel for
	for (size_t ii = 0; ii< blockcount; ii++) {
		if (controlbitvector[ii]) {
			#pragma omp simd aligned(d,b,z:16)
			for (size_t jj = 0; jj < fullblocksize; jj++) {
				d[ii * fullblocksize + jj] ^= b[ii * fullblocksize + jj] ^ z[jj];
			}
		} else {
			#pragma omp simd aligned(d,b:16)
			for (size_t jj = 0; jj < fullblocksize; jj++) {
				d[ii * fullblocksize + jj] ^= b[ii * fullblocksize + jj];
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