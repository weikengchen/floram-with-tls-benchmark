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


void scanrom_encrypt_offline(uint8_t * out, uint8_t * in, uint8_t* key, size_t index, size_t blockmultiple, size_t blockcount) {
#define OCTBLOCK(II,JJ) &o[(II+JJ)*BLOCKSIZE] 

	uint8_t * o = out;
	uint8_t * i = in;

	void * kex = offline_prf_keyschedule(key);
	__m128i mask = _mm_set_epi64((__m64)0x08090a0b0c0d0e0fULL, (__m64)0x0001020304050607ULL );
	__m128i mr1, mr2, mr3, mr4, mr5, mr6, mr7, mr8;

	if (in == NULL) {
		size_t ii;
		#pragma omp parallel for
		for (ii = index*blockmultiple; ii < ((index+blockcount)*blockmultiple)-((index+blockcount)*blockmultiple)%8; ii+= 8) {
		    mr1 = _mm_shuffle_epi8 (_mm_set_epi64((__m64)ii,(__m64)0l), mask);
		    mr2 = _mm_shuffle_epi8 (_mm_set_epi64((__m64)ii+1,(__m64)0l), mask);
		    mr3 = _mm_shuffle_epi8 (_mm_set_epi64((__m64)ii+2,(__m64)0l), mask);
		    mr4 = _mm_shuffle_epi8 (_mm_set_epi64((__m64)ii+3,(__m64)0l), mask);
		    mr5 = _mm_shuffle_epi8 (_mm_set_epi64((__m64)ii+4,(__m64)0l), mask);
		    mr6 = _mm_shuffle_epi8 (_mm_set_epi64((__m64)ii+5,(__m64)0l), mask);
		    mr7 = _mm_shuffle_epi8 (_mm_set_epi64((__m64)ii+6,(__m64)0l), mask);
		    mr8 = _mm_shuffle_epi8 (_mm_set_epi64((__m64)ii+7,(__m64)0l), mask);
			offline_prf_oct(OCTBLOCK(ii,0),OCTBLOCK(ii,1),OCTBLOCK(ii,2),OCTBLOCK(ii,3),OCTBLOCK(ii,4),OCTBLOCK(ii,5),OCTBLOCK(ii,6),OCTBLOCK(ii,7),
							&mr1, &mr2, &mr3, &mr4, &mr5, &mr6, &mr7, &mr8, kex, kex, kex, kex, kex, kex, kex, kex);
		}
		for (; ii < ((index+blockcount)*blockmultiple); ii+= 1) {
			__m128i mr1 = _mm_shuffle_epi8 (_mm_set_epi64((__m64)ii,(__m64)0l), mask);
			offline_prf(&o[ii*BLOCKSIZE], &mr1, kex);
		}
	} else {
		size_t ii;
		#pragma omp parallel for
		for (ii = index*blockmultiple; ii < ((index+blockcount)*blockmultiple)-((index+blockcount)*blockmultiple)%8; ii+= 8) {
		    mr1 = _mm_shuffle_epi8 (_mm_set_epi64((__m64)ii,(__m64)0l), mask);
		    mr2 = _mm_shuffle_epi8 (_mm_set_epi64((__m64)ii+1,(__m64)0l), mask);
		    mr3 = _mm_shuffle_epi8 (_mm_set_epi64((__m64)ii+2,(__m64)0l), mask);
		    mr4 = _mm_shuffle_epi8 (_mm_set_epi64((__m64)ii+3,(__m64)0l), mask);
		    mr5 = _mm_shuffle_epi8 (_mm_set_epi64((__m64)ii+4,(__m64)0l), mask);
		    mr6 = _mm_shuffle_epi8 (_mm_set_epi64((__m64)ii+5,(__m64)0l), mask);
		    mr7 = _mm_shuffle_epi8 (_mm_set_epi64((__m64)ii+6,(__m64)0l), mask);
		    mr8 = _mm_shuffle_epi8 (_mm_set_epi64((__m64)ii+7,(__m64)0l), mask);
			offline_prf_oct(OCTBLOCK(ii,0),OCTBLOCK(ii,1),OCTBLOCK(ii,2),OCTBLOCK(ii,3),OCTBLOCK(ii,4),OCTBLOCK(ii,5),OCTBLOCK(ii,6),OCTBLOCK(ii,7),
							&mr1, &mr2, &mr3, &mr4, &mr5, &mr6, &mr7, &mr8, kex, kex, kex, kex, kex, kex, kex, kex);
			#pragma omp simd aligned(o:16)
			for (size_t jj = ii*BLOCKSIZE; jj < (ii+8)*BLOCKSIZE; jj++) {
				o[jj] ^= i[jj];
			}
		}
		for (; ii < ((index+blockcount)*blockmultiple); ii+= 1) {
			__m128i mr1 = _mm_shuffle_epi8 (_mm_set_epi64((__m64)ii,(__m64)0l), mask);
			offline_prf(&o[ii*BLOCKSIZE], &mr1, kex);
			#pragma omp simd aligned(o,i:16)
			for (size_t jj = ii*BLOCKSIZE; jj < (ii+1)*BLOCKSIZE; jj++) {
				o[jj] ^= i[jj];
			}
		}
	}

	free(kex);
}


void scanwrom_write_with_blockvector_offline(uint8_t * local_data, uint8_t * blockvector, bool * controlbitvector, uint8_t*Zblock, size_t memblocksize, size_t blockcount) {
	uint64_t * d = local_data;
	uint64_t * b = blockvector;
	uint64_t * z = Zblock;

	#pragma omp parallel for
	for (size_t ii = 0; ii< blockcount; ii++) {
		if (controlbitvector[ii]) {
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