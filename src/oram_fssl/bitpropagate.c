#include "bitpropagate.h"
#include "floram_util.h"
#include "ackutil.h"
#include <omp.h>

struct bitpropagator_offline {
	size_t size;
	size_t startlevel;
	size_t endlevel;
	void * Z;
	bool * advicebits_l;
	bool * advicebits_r;
	void * level_data;
	void * level_bits;
	void * keyL;
	void * keyR;
	omp_lock_t * locks;
};

void bitpropagator_offline_start(bitpropagator_offline * bpo, void * blocks) {
	memcpy(bpo->level_data, blocks, (1ll<<bpo->startlevel) * BLOCKSIZE);
	for (int ii = 0; ii < (bpo->endlevel - bpo->startlevel); ii++) {
		omp_set_lock(&bpo->locks[ii]);
	}
}

void bitpropagator_offline_push_Z(bitpropagator_offline * bpo, void * Z, bool advicebit_l, bool advicebit_r, size_t level) {
	memcpy(&bpo->Z[(level- bpo->startlevel - 1)*BLOCKSIZE], Z, BLOCKSIZE);
	bpo->advicebits_l[level- bpo->startlevel - 1] = advicebit_l;
	bpo->advicebits_r[level- bpo->startlevel - 1] = advicebit_r;
	omp_unset_lock(&bpo->locks[level- bpo->startlevel - 1]);
}

void bitpropagator_offline_readblockvector(void * local_output, void * local_bit_output, bitpropagator_offline * bpo) {

	size_t thislevel = bpo->startlevel;
	size_t thislevelblocks = (1ll<<bpo->startlevel);
	size_t nextlevelblocks = (bpo->size + (1ll<<(bpo->endlevel - thislevel -1)) - 1) / (1ll<<(bpo->endlevel - thislevel -1));

	uint64_t* a = (uint64_t *)bpo->level_data;
	uint8_t* a2 = (uint8_t *)bpo->level_data;
	uint64_t* b = (uint64_t *)local_output;
	uint8_t* b2 = (uint8_t *)local_output;
	uint64_t* t;
	uint8_t* t2;
	uint64_t* z;
	bool advicebit_l, advicebit_r;
	bool * a_bits = bpo->level_bits;
	bool * b_bits = local_bit_output;
	bool * t_bits;

	#pragma omp parallel for
	for (size_t ii = 0; ii < thislevelblocks; ii++) {
		if ((ii+1)*2 <= nextlevelblocks) {
			offline_prf(&b2[ii*2*BLOCKSIZE], &a2[ii*BLOCKSIZE], bpo->keyL);
			offline_prf(&b2[(ii*2+1)*BLOCKSIZE], &a2[ii*BLOCKSIZE], bpo->keyR);
		} else if (ii*2+1 <= nextlevelblocks) {
			offline_prf(&b2[ii*2*BLOCKSIZE], &a2[ii*BLOCKSIZE], bpo->keyL);
		}
		a_bits[ii] = a2[ii*BLOCKSIZE] & 1;
	}

	for (thislevel = bpo->startlevel +1; thislevel < bpo->endlevel; thislevel++) {
		omp_set_lock(&bpo->locks[thislevel- bpo->startlevel -1 ]);

		thislevelblocks = nextlevelblocks;
		nextlevelblocks = (bpo->size + (1ll<<(bpo->endlevel - thislevel -1)) - 1) / (1ll<<(bpo->endlevel - thislevel -1));
		if (thislevel == bpo->endlevel -1) nextlevelblocks = bpo->size;

		advicebit_l = bpo->advicebits_l[thislevel - bpo->startlevel -1];
		advicebit_r = bpo->advicebits_r[thislevel - bpo->startlevel -1];

		z = &((uint64_t *)bpo->Z)[(thislevel - bpo->startlevel -1) * (BLOCKSIZE/sizeof(uint64_t))];

		t2 = b2; t = b; t_bits = b_bits;
		b2 = a2; b = a; b_bits = a_bits;
		a2 = t2; a = t; a_bits = t_bits;

		#pragma omp parallel for
		for (int64_t ii =  0; ii < thislevelblocks ; ii++) {

			if (ii%2 == 0) {
				a_bits[ii] = (a2[ii*BLOCKSIZE] & 1) ^ (b_bits[ii/2] & advicebit_l);
			} else {
				a_bits[ii] = (a2[ii*BLOCKSIZE] & 1) ^ (b_bits[ii/2] & advicebit_r);
			}

			if (b_bits[ii/2]) {
				#pragma omp simd aligned(a,z:16)
				for (uint8_t jj = 0; jj < BLOCKSIZE/sizeof(uint64_t); jj++) {
					a[ii*(BLOCKSIZE/sizeof(uint64_t))+jj] ^= z[jj];
				}
			}
		
			if ((ii+1)*2 <= nextlevelblocks) {
				offline_prf(&b2[ii*2*BLOCKSIZE], &a2[ii*BLOCKSIZE], bpo->keyL);
				offline_prf(&b2[(ii*2+1)*BLOCKSIZE], &a2[ii*BLOCKSIZE], bpo->keyR);
			} else if (ii*2+1 <= nextlevelblocks) {
				offline_prf(&b2[ii*2*BLOCKSIZE], &a2[ii*BLOCKSIZE], bpo->keyL);
			}
		}
	}

	omp_set_lock(&bpo->locks[thislevel- bpo->startlevel -1 ]);

	thislevelblocks = nextlevelblocks;

	advicebit_l = bpo->advicebits_l[thislevel - bpo->startlevel -1];
	advicebit_r = bpo->advicebits_r[thislevel - bpo->startlevel -1];

	z = &((uint64_t *)bpo->Z)[(thislevel - bpo->startlevel -1) * (BLOCKSIZE/sizeof(uint64_t))];

	t2 = b2; t = b; t_bits = b_bits;
	b2 = a2; b = a; b_bits = a_bits;
	a2 = t2; a = t; a_bits = t_bits;

	if (b == local_output) {
		#pragma omp parallel for
		for (size_t ii = 0; ii < thislevelblocks; ii++) {
			if (ii%2 == 0) {
				a_bits[ii] = (a2[ii*BLOCKSIZE] & 1) ^ (b_bits[ii/2] & advicebit_l);
			} else {
				a_bits[ii] = (a2[ii*BLOCKSIZE] & 1) ^ (b_bits[ii/2] & advicebit_r);
			}

			if (b_bits[ii/2]) {
				#pragma omp simd aligned(b,a,z:16)
				for (uint8_t jj = 0; jj < BLOCKSIZE/sizeof(uint64_t); jj++) {
					b[ii*(BLOCKSIZE/sizeof(uint64_t))+jj] = a[ii*(BLOCKSIZE/sizeof(uint64_t))+jj] ^ z[jj];
				}
			} else {
				memcpy(&b[ii*(BLOCKSIZE/sizeof(uint64_t))], &a[ii*(BLOCKSIZE/sizeof(uint64_t))], BLOCKSIZE);
			}
		}

		memcpy(b_bits, a_bits, thislevelblocks*sizeof(bool));

	} else {
		#pragma omp parallel for
		for (size_t ii = 0; ii < thislevelblocks; ii++) {
			if (ii%2 == 0) {
				a_bits[ii] = (a2[ii*BLOCKSIZE] & 1) ^ (b_bits[ii/2] & advicebit_l);
			} else {
				a_bits[ii] = (a2[ii*BLOCKSIZE] & 1) ^ (b_bits[ii/2] & advicebit_r);
			}

			if (b_bits[ii/2]) {
				#pragma omp simd aligned(b,a,z:16)
				for (uint8_t jj = 0; jj < BLOCKSIZE/sizeof(uint64_t); jj++) {
					a[ii*(BLOCKSIZE/sizeof(uint64_t))+jj] ^= z[jj];
				}
			}
		}
	}

	for (int ii = 0; ii < (bpo->endlevel - bpo->startlevel); ii++) {
		omp_unset_lock(&bpo->locks[ii]);
	}
}

void bitpropagator_offline_parallelizer(void* bp, bitpropagator_offline * bpo, void* indexp, void * local_output, void * local_bit_output, void* pd, bp_traverser_fn fn, bp_pusher_fn fn2, facb_fn cbfn, void* cbpass) {

	omp_set_nested(true);

	#pragma omp parallel num_threads(3)
	{
		//OpenMP seems to get along with obliv-c just fine, so long as obliv-c only uses the master thread.
		#pragma omp master
		{
			fn(bp, indexp);	
			if (*cbfn!=NULL) {
				cbfn(cbpass, NULL);
			}
		}
		

		#pragma omp single
		{
			#pragma omp task
			fn2(bp, bpo, pd);

			#pragma omp task
			bitpropagator_offline_readblockvector(local_output, local_bit_output, bpo);
		}
	}
}

bitpropagator_offline * bitpropagator_offline_new(size_t size, size_t startlevel, uint8_t * keyL, uint8_t * keyR) {
	offline_expand_init();
	bitpropagator_offline * bpo = malloc(sizeof(bitpropagator_offline));
	bpo->size = size;
	bpo->startlevel = startlevel;
	bpo->endlevel = LOG2LL(size) + (((1 << LOG2LL(size)) < size)? 1:0);
	posix_memalign(&bpo->level_data,16,(1ll<<bpo->endlevel) * BLOCKSIZE);
	posix_memalign(&bpo->Z,16,(bpo->endlevel - bpo->startlevel) * BLOCKSIZE);
	bpo->locks = malloc((bpo->endlevel - bpo->startlevel) * sizeof(omp_lock_t));
	bpo->advicebits_l = malloc((bpo->endlevel - bpo->startlevel) * sizeof(bool));
	bpo->advicebits_r = malloc((bpo->endlevel - bpo->startlevel) * sizeof(bool));
	bpo->level_bits = malloc(size * sizeof(bool));

	bpo->keyL = offline_prf_keyschedule(keyL);
	bpo->keyR = offline_prf_keyschedule(keyR); 

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
	free(bpo->level_data);
	free(bpo->level_bits);
	free(bpo->advicebits_l);
	free(bpo->advicebits_r);
	free(bpo->Z);
	free(bpo->keyL);
	free(bpo->keyR);
	free(bpo->locks);
	free(bpo);
}