#include "scanrom.h"
#include "flatoram_util.h"
#include <wmmintrin.h>
#include <tmmintrin.h>

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

void scanrom_encrypt_offline(uint8_t * out, uint8_t * in, uint8_t* key, size_t index, size_t len) {
	uint8_t * o = out;
	uint8_t * i = in;

	void * kex = offline_prf_keyschedule(key);

	if (in == NULL) {
		#pragma omp parallel for
		for (size_t ii = index/BLOCKSIZE; ii < (index+len) / BLOCKSIZE; ii++) {
			__m128i mask = _mm_set_epi64((__m64)0x08090a0b0c0d0e0fULL, (__m64)0x0001020304050607ULL );
		    __m128i mr = _mm_shuffle_epi8 (_mm_set_epi64((__m64)ii,(__m64)0l), mask);
			offline_prf(&o[ii*BLOCKSIZE], &mr, kex);
		}
	} else {
		#pragma omp parallel for
		for (size_t ii = index/BLOCKSIZE; ii < (index+len) / BLOCKSIZE; ii++) {
			__m128i mask = _mm_set_epi64((__m64)0x08090a0b0c0d0e0fULL, (__m64)0x0001020304050607ULL );
		    __m128i mr = _mm_shuffle_epi8 (_mm_set_epi64((__m64)ii,(__m64)0l), mask);
			offline_prf(&o[ii*BLOCKSIZE], &mr, kex);
			#pragma omp simd aligned(o,i:16)
			for (uint8_t jj=0;jj<BLOCKSIZE;jj++) {
				o[ii*BLOCKSIZE+jj] ^= i[ii*BLOCKSIZE+jj];
			}
		}
	}

	
	free(kex);
}


void scanwrom_write_with_blockvector_offline(uint8_t * local_data, uint8_t * blockvector, bool * controlbitvector, uint8_t*Zblock, bool expand, size_t fullblocksize, size_t blockcount) {
	uint64_t * d = local_data;
	uint64_t * b = blockvector;
	uint64_t * z = Zblock;

	#pragma omp parallel for
	for (size_t ii = 0; ii< blockcount; ii++) {
		if (controlbitvector[ii]) {
			#pragma omp simd aligned(d,b,z:16)
			for (size_t jj = 0; jj < fullblocksize/sizeof(uint64_t); jj++) {
				d[ii * fullblocksize/sizeof(uint64_t) + jj] ^= b[ii * fullblocksize/sizeof(uint64_t) + jj] ^ z[jj];
			}
		} else {
			#pragma omp simd aligned(d,b:16)
			for (size_t jj = 0; jj < fullblocksize/sizeof(uint64_t); jj++) {
				d[ii * fullblocksize/sizeof(uint64_t) + jj] ^= b[ii * fullblocksize/sizeof(uint64_t) + jj];
			}
		}
	}
}


// Unfinished
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