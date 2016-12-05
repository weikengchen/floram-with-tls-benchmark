#include "bitpropagate.h"
#include "flatoram_util.h"
#include "ackutil.h"
#include <omp.h>

struct bitpropagator_offline {
	size_t size;
	size_t startlevel;
	size_t endlevel;
	void * Z;
	uint32_t * advicebits;
	void * level_data_1;
	void * level_data_2;
	omp_lock_t * locks;
};

void bitpropagator_offline_start(bitpropagator_offline * bpo, void * blocks) {
	memcpy(bpo->level_data_1, blocks, (1ll<<bpo->startlevel) * BLOCKSIZE);
	for (int ii = 0; ii < (bpo->endlevel - bpo->startlevel); ii++) {
		omp_set_lock(&bpo->locks[ii]);
	}
}

void bitpropagator_offline_push_Z(bitpropagator_offline * bpo, void * Z, uint32_t advicebit, size_t level) {
	#pragma omp task
	{
		memcpy(&bpo->Z[(level- bpo->startlevel - 1)*BLOCKSIZE], Z, BLOCKSIZE);
		bpo->advicebits[level- bpo->startlevel - 1] = advicebit;
		omp_unset_lock(&bpo->locks[level- bpo->startlevel - 1]);
	}
}

void bitpropagator_offline_readblockvector(void * local_output, bitpropagator_offline * bpo) {

	size_t thislevel = bpo->startlevel;
	size_t thislevelblocks = (1ll<<bpo->startlevel);
	size_t nextlevelblocks = (bpo->size + (1ll<<(bpo->endlevel - thislevel -1)) - 1) / (1ll<<(bpo->endlevel - thislevel -1));

	uint64_t* a = (uint64_t *)bpo->level_data_1;
	uint8_t* a2 = (uint8_t *)bpo->level_data_1;
	uint64_t* b = (uint64_t *)bpo->level_data_2;
	uint8_t* b2 = (uint8_t *)bpo->level_data_2;
	uint64_t* t;
	uint8_t* t2;
	uint64_t* z;
	uint8_t abyte;
	uint8_t abit;

	#pragma omp parallel for
	for (size_t ii = 0; ii < thislevelblocks; ii++) {
		if ((ii+1)*2 <= nextlevelblocks) {
			offline_expand(&b2[ii*2*BLOCKSIZE], &a2[ii*BLOCKSIZE], 2);
		} else if (ii*2+1 <= nextlevelblocks) {
			offline_expand(&b2[ii*2*BLOCKSIZE], &a2[ii*BLOCKSIZE], 1);
		}
	}


	for (thislevel = bpo->startlevel +1; thislevel < bpo->endlevel; thislevel++) {
		omp_set_lock(&bpo->locks[thislevel- bpo->startlevel -1 ]);

		thislevelblocks = nextlevelblocks;
		nextlevelblocks = (bpo->size + (1ll<<(bpo->endlevel - thislevel -1)) - 1) / (1ll<<(bpo->endlevel - thislevel -1));
		if (thislevel == bpo->endlevel -1) nextlevelblocks = bpo->size;

		abyte = bpo->advicebits[thislevel - bpo->startlevel -1]/8;
		abit = bpo->advicebits[thislevel - bpo->startlevel -1]%8;

		z = &((uint64_t *)bpo->Z)[(thislevel - bpo->startlevel -1) * (BLOCKSIZE/sizeof(uint64_t))];

		t2 = b2; t = b;
		b2 = a2; b = a;
		a2 = t2; a = t;

		#pragma omp parallel for
		for (int64_t ii =  0; ii < thislevelblocks ; ii++) {
			if ((a2[ii*BLOCKSIZE + abyte] >> abit) & 1 == 1) {
				#pragma omp simd aligned(a,z:16)
				for (uint8_t jj = 0; jj < BLOCKSIZE/sizeof(uint64_t); jj++) {
					a[ii*(BLOCKSIZE/sizeof(uint64_t))+jj] ^= z[jj];
				}
			}
		
			if ((ii+1)*2 <= nextlevelblocks) {
				offline_expand(&b2[ii*2*BLOCKSIZE], &a2[ii*BLOCKSIZE], 2);
			} else if (ii*2+1 <= nextlevelblocks) {
				offline_expand(&b2[ii*2*BLOCKSIZE], &a2[ii*BLOCKSIZE], 1);
			}
		}
	}

	uint64_t* c = (uint64_t *)local_output;

	omp_set_lock(&bpo->locks[thislevel- bpo->startlevel -1 ]);

	thislevelblocks = nextlevelblocks;

	abyte = bpo->advicebits[thislevel - bpo->startlevel -1]/8;
	abit = bpo->advicebits[thislevel - bpo->startlevel -1]%8;

	z = &((uint64_t *)bpo->Z)[(thislevel - bpo->startlevel -1) * (BLOCKSIZE/sizeof(uint64_t))];

	#pragma omp parallel for
	for (size_t ii = 0; ii < thislevelblocks; ii++) {
		if ((b2[ii*BLOCKSIZE + abyte] >> abit) & 1 == 1) {
			#pragma omp simd aligned(z,b,c:16)
			for (uint8_t jj = 0; jj < BLOCKSIZE/sizeof(uint64_t); jj++) {
				c[ii*(BLOCKSIZE/sizeof(uint64_t))+jj] = b[ii*(BLOCKSIZE/sizeof(uint64_t))+jj] ^ z[jj];
			}
		} else {
			memcpy(&c[ii*(BLOCKSIZE/sizeof(uint64_t))],&b[ii*(BLOCKSIZE/sizeof(uint64_t))],BLOCKSIZE);
		}
	}

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

bitpropagator_offline * bitpropagator_offline_new(size_t size, size_t startlevel) {
	offline_expand_init();
	bitpropagator_offline * bpo = malloc(sizeof(bitpropagator_offline));
	bpo->size = size;
	bpo->startlevel = startlevel;
	bpo->endlevel = LOG2LL(size) + (((1 << LOG2LL(size)) < size)? 1:0);
	posix_memalign(&bpo->level_data_1,16,(1ll<<bpo->endlevel) * BLOCKSIZE);
	posix_memalign(&bpo->level_data_2,16,(1ll<<bpo->endlevel) * BLOCKSIZE);
	posix_memalign(&bpo->Z,16,(bpo->endlevel - bpo->startlevel) * BLOCKSIZE);
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
	free(bpo->level_data_1);
	free(bpo->level_data_2);
	free(bpo->advicebits);
	free(bpo->Z);
	free(bpo->locks);
	free(bpo);
}

void bitpropagator_offline_applyadvice(bool * bitflags, uint8_t * local_data, size_t blocksize, size_t blockcount, int32_t advice) {
	#pragma omp parallel for
	for (size_t ii = 0; ii < blockcount; ii++) {
		bitflags[ii] = (local_data[ii * blocksize + advice/8] >> (advice % 8)) & 1;
	}
}