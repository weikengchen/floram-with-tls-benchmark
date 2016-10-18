#include "bitpropagate.h"
#include "flatoram_util.h"
#include <omp.h>

struct bitpropagator_offline {
	size_t size;
	uint32_t startlevel;
	uint32_t endlevel;
	void * Z;
	uint32_t * advicebits;
	void * startlevel_data;
	void * nextlevel_data;
	void * thislevel_data;
	omp_lock_t * locks;
};

void bitpropagator_offline_start(bitpropagator_offline * bpo, void * blocks) {
	memcpy(bpo->thislevel_data, blocks, (1<<bpo->startlevel) * BLOCKSIZE);
	for (int ii = 0; ii < (bpo->endlevel - bpo->startlevel); ii++) {
		omp_set_lock(&bpo->locks[ii]);
	}
}

void bitpropagator_offline_push_Z(bitpropagator_offline * bpo, void * Z, uint32_t advicebit, uint32_t level) {
	memcpy(&bpo->Z[(level- bpo->startlevel - 1)*BLOCKSIZE], Z, BLOCKSIZE);
	bpo->advicebits[level- bpo->startlevel - 1] = advicebit;
	omp_unset_lock(&bpo->locks[level- bpo->startlevel - 1]);
}

void bitpropagator_offline_readblockvector(void * local_output, bitpropagator_offline * bpo) {
	
	for (uint32_t thislevel = bpo->startlevel; thislevel < bpo->endlevel; thislevel++) {
		size_t maxblocks = (bpo->size + (1<<(bpo->endlevel - thislevel -1)) - 1) / (1<<(bpo->endlevel - thislevel -1));

		#pragma omp parallel for
		for (size_t ii = 0; ii < maxblocks; ii += 2) {
			if (ii+2 <= maxblocks) {
				offline_expand(&bpo->nextlevel_data[ii*BLOCKSIZE], &bpo->thislevel_data[ii/2*BLOCKSIZE], 2);
			} else {
				offline_expand(&bpo->nextlevel_data[ii*BLOCKSIZE], &bpo->thislevel_data[ii/2*BLOCKSIZE], 1);
			}
		}

		memcpy(bpo->thislevel_data, bpo->nextlevel_data, maxblocks*BLOCKSIZE);
		omp_set_lock(&bpo->locks[thislevel- bpo->startlevel]);

		#pragma omp parallel for
		for (size_t ii = 0; ii < maxblocks; ii++) {
			uint8_t abyte = bpo->advicebits[thislevel - bpo->startlevel]/8;
			uint8_t abit = bpo->advicebits[thislevel - bpo->startlevel]%8;
			if ((((uint8_t *)bpo->thislevel_data)[ii*BLOCKSIZE + abyte] >> abit) & 1 == 1) {
				uint64_t* a = &((uint64_t *)bpo->thislevel_data)[ii*(BLOCKSIZE/sizeof(uint64_t))];
				uint64_t* b = &((uint64_t *)bpo->Z)[(thislevel - bpo->startlevel) * (BLOCKSIZE/sizeof(uint64_t))];
				#pragma omp simd 
				for (uint8_t jj = 0; jj < BLOCKSIZE/sizeof(uint64_t); jj++) {
					a[jj] ^= b[jj];
				}
			}
		}
	}
	
	memcpy(local_output, bpo->thislevel_data, bpo->size * BLOCKSIZE);

	for (int ii = 0; ii < (bpo->endlevel - bpo->startlevel); ii++) {
		omp_unset_lock(&bpo->locks[ii]);
	}
}

void bitpropagator_offline_parallelizer(void* bp, bitpropagator_offline * bpo, void* indexp, void * local_output, bp_traverser_fn fn) {
	omp_set_nested(true);

	#pragma omp parallel num_threads(2)
	{
		//OpenMP seems to get along with obliv-c just fine, so long as obliv-c only uses the master thread.
		#pragma omp master
		fn(bp, indexp);	

		#pragma omp single
		{
			#pragma omp task
			bitpropagator_offline_readblockvector(local_output, bpo);
		}
	}
}

bitpropagator_offline * bitpropagator_offline_new(size_t size, uint32_t startlevel) {
	offline_expand_init();
	bitpropagator_offline * bpo = malloc(sizeof(bitpropagator_offline));
	bpo->size = size;
	bpo->startlevel = startlevel;
	bpo->endlevel = LOG2(size) + (((1 << LOG2(size)) < size)? 1:0);
	bpo->startlevel_data = malloc((1<<bpo->startlevel) * BLOCKSIZE);
	bpo->nextlevel_data = malloc((1<<bpo->endlevel) * BLOCKSIZE);
	bpo->thislevel_data = malloc((1<<bpo->endlevel) * BLOCKSIZE);
	bpo->Z = malloc((bpo->endlevel - bpo->startlevel)*BLOCKSIZE);
	bpo->locks = malloc((bpo->endlevel - bpo->startlevel) * sizeof(omp_lock_t));
	bpo->advicebits = malloc((bpo->endlevel - bpo->startlevel) * sizeof(uint32_t));
	for (int ii = 0; ii < (bpo->endlevel - bpo->startlevel); ii++) {
		omp_init_lock(&bpo->locks[ii]);
	}
	return bpo;
}

void bitpropagator_offline_free(bitpropagator_offline * bpo) {
	for (int ii = 0; ii < (bpo->endlevel - bpo->startlevel); ii++) {
		omp_destroy_lock(&bpo->locks[ii]);
	}
	offline_expand_deinit();
	free(bpo->startlevel_data);
	free(bpo->nextlevel_data);
	free(bpo->thislevel_data);
	free(bpo->advicebits);
	free(bpo->Z);
	free(bpo->locks);
	free(bpo);
}