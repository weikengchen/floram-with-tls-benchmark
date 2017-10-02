#include "scanrom.h"
#include "floram_util.h"


void scanrom_read_with_bitvector_offline(uint8_t * output_share, uint8_t * rom_memory, bool * bitvector, size_t memblocksize, size_t blockcount) {
	memset(output_share, 0, memblocksize);

	uint64_t * rm = rom_memory;
	bool * biv = bitvector;

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
			#pragma omp simd aligned(rm,biv,s:16)
			for (size_t jj = 0; jj < memblocksize /sizeof(uint64_t); jj++) {
				s[jj] ^= biv[ii] * rm[ii * ((memblocksize) /sizeof(uint64_t)) + jj];
			}
		}
	}

	for (size_t ii = 0; ii < threadcount; ii++) {
		for (size_t jj = 0; jj < memblocksize /sizeof(uint64_t); jj++) {
			((uint64_t *)output_share)[jj] ^= sums[ii][jj];
		}
		free(sums[ii]);
	}

	free(sums);
}

void scanrom_read_with_blockvector_offline(uint8_t * z_and_output, uint8_t * rom_memory, bool * bitvector, uint8_t * blockvector, size_t memblocksize, size_t blockcount) {
	uint64_t * rm = rom_memory;
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
			bool bitvector_bit = bitvector[ii/(BLOCKSIZE * 8)];
			uint8_t z_block_part = z_and_output[(ii%(BLOCKSIZE*8))/8];
			uint8_t * blockvector_block = &blockvector[(ii/(BLOCKSIZE * 8))*BLOCKSIZE];
			uint8_t blockvector_block_part = blockvector_block[(ii%(BLOCKSIZE*8))/8];
			bool b_temp = (((bitvector_bit * z_block_part) ^ blockvector_block_part) >> (ii % 8)) & 0x1;
			#pragma omp simd aligned(rm,s:16)
			for (size_t jj = 0; jj < memblocksize /sizeof(uint64_t); jj++) {
				s[jj] ^= b_temp * rm[ii * memblocksize/sizeof(uint64_t) + jj];
			}
		}
	}

	memset(z_and_output, 0, memblocksize);

	for (size_t ii = 0; ii < threadcount; ii++) {
		for (size_t jj = 0; jj < memblocksize /sizeof(uint64_t); jj++) {
			((uint64_t *)z_and_output)[jj] ^= sums[ii][jj];
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

void scanwrom_write_with_blockvector_offline(uint8_t * wrom_memory, uint8_t * blockvector, bool * bitvector, uint8_t*z_block, size_t memblocksize, size_t blockcount) {
	uint64_t * wm = wrom_memory;
	uint64_t * blv = blockvector;
	bool * biv = bitvector;
	uint64_t * z = z_block;

	floram_set_procs_for_data_size(memblocksize * blockcount);
	#pragma omp parallel for schedule(guided)
	for (size_t ii = 0; ii< blockcount; ii++) {
		#pragma omp simd aligned(wm,blv,biv,z:16)
		for (size_t jj = 0; jj < memblocksize/sizeof(uint64_t); jj++) {
			wm[ii * memblocksize/sizeof(uint64_t) + jj] ^= blv[ii * memblocksize/sizeof(uint64_t) + jj] ^ (biv[ii] * z[jj]);
		}
	}
}