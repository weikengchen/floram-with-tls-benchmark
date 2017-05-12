#include "scanrom.h"
#include "floram_util.h"


void scanrom_read_with_bitvector_offline(uint8_t * data, uint8_t * local_data, bool * bitvector, size_t memblocksize, size_t blockcount) {
	memset(data, 0, memblocksize);

	uint64_t * d = local_data;
	bool * b = bitvector;

	uint64_t ** sums;
	size_t threadcount;

	floram_set_procs_for_data_size(memblocksize * blockcount);
	#pragma omp parallel 
	{
		threadcount = omp_get_num_threads();

		#pragma omp single
		sums = malloc(threadcount * sizeof(uint64_t *));

		uint64_t * s;
		floram_zpma(&s, 16, memblocksize);
		sums[omp_get_thread_num()] = s;

		#pragma omp for schedule(guided)
		for (size_t ii = 0; ii < blockcount; ii++) {
			#pragma omp simd aligned(d,b,s:16)
			for (size_t jj = 0; jj < memblocksize /sizeof(uint64_t); jj++) {
				s[jj] ^= b[ii] * d[ii * ((memblocksize) /sizeof(uint64_t)) + jj];
			}
		}
	}

	for (size_t ii = 0; ii < threadcount; ii++) {
		for (size_t jj = 0; jj < memblocksize /sizeof(uint64_t); jj++) {
			((uint64_t *)data)[jj] ^= sums[ii][jj];
		}
		free(sums[ii]);
	}

	free(sums);
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
	offline_expand_from(out, key, index*blockmultiple, blockcount * blockmultiple);
	if (in != NULL) {
		floram_set_procs_for_data_size(BLOCKSIZE * blockmultiple * blockcount);
		#pragma omp parallel for simd schedule(guided)
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

	floram_set_procs_for_data_size(memblocksize * blockcount);
	#pragma omp parallel for schedule(guided)
	for (size_t ii = 0; ii< blockcount; ii++) {
		#pragma omp simd aligned(d,b,bitvector,z:16)
		for (size_t jj = 0; jj < memblocksize/sizeof(uint64_t); jj++) {
			d[ii * memblocksize/sizeof(uint64_t) + jj] ^= b[ii * memblocksize/sizeof(uint64_t) + jj] ^ (bitvector[ii] * z[jj]);
		}
	}
}