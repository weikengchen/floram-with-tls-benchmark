#include "bitpropagate.h"
#include "floram_util.h"
#include "ackutil.h"
#include <omp.h>

struct bitpropagator_offline {
	size_t size;
	size_t blockmultiple;
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

void bitpropagator_offline_start(bitpropagator_offline * bpo, uint8_t * blocks) {
	memcpy(bpo->level_data, blocks, (1ll<<bpo->startlevel) * BLOCKSIZE);
	for (int ii = 0; ii < (bpo->endlevel - bpo->startlevel); ii++) {
		omp_set_lock(&bpo->locks[ii]);
	}
}

void bitpropagator_offline_push_Z(bitpropagator_offline * bpo, uint8_t * Z, bool advicebit_l, bool advicebit_r, size_t level) {
	memcpy(&bpo->Z[(level- bpo->startlevel - 1)*BLOCKSIZE], Z, BLOCKSIZE);
	bpo->advicebits_l[level- bpo->startlevel - 1] = advicebit_l;
	bpo->advicebits_r[level- bpo->startlevel - 1] = advicebit_r;
	omp_unset_lock(&bpo->locks[level- bpo->startlevel - 1]);
}

void bitpropagator_offline_readblockvector(uint8_t * local_output, bool * local_bit_output, bitpropagator_offline * bpo) {	

	floram_set_procs_for_data_size(BLOCKSIZE * (1ll<<bpo->endlevel + 1ll<<(bpo->endlevel-1)));
	#pragma omp parallel
	{
		size_t thislevel = bpo->startlevel;
		size_t thislevelblocks = (1ll<<bpo->startlevel);
		size_t nextlevelblocks = (bpo->size + (1ll<<(bpo->endlevel - thislevel -1)) - 1) / (1ll<<(bpo->endlevel - thislevel -1));
		size_t expansion_stride;

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

		if (thislevel == bpo->endlevel - 1 && b2 == local_output) {
			expansion_stride = (BLOCKSIZE * bpo->blockmultiple);
		} else {
			expansion_stride = BLOCKSIZE;
		}

#ifdef ORAM_PROFILE_SCHEDULING
		#pragma omp single nowait
		printf("START FSS OFFLINE LEVEL 0 %lld\n", current_timestamp());
#endif

		#pragma omp for schedule(guided)
		for (size_t ii = 0; ii < 4*(nextlevelblocks/8); ii+=4) {
			offline_prg_oct(&b2[ii*2*expansion_stride], &b2[(ii*2+1)*expansion_stride], &b2[(ii*2+2)*expansion_stride], &b2[(ii*2+3)*expansion_stride],
							&b2[(ii*2+4)*expansion_stride], &b2[(ii*2+5)*expansion_stride], &b2[(ii*2+6)*expansion_stride], &b2[(ii*2+7)*expansion_stride],
							&a2[ii*BLOCKSIZE],  &a2[ii*BLOCKSIZE], &a2[(ii+1)*BLOCKSIZE], &a2[(ii+1)*BLOCKSIZE],
							&a2[(ii+2)*BLOCKSIZE], &a2[(ii+2)*BLOCKSIZE],&a2[(ii+3)*BLOCKSIZE], &a2[(ii+3)*BLOCKSIZE],
							bpo->keyL, bpo->keyR, bpo->keyL, bpo->keyR,
							bpo->keyL, bpo->keyR, bpo->keyL, bpo->keyR);
			a_bits[ii] = a2[ii*BLOCKSIZE] & 1;
			a_bits[ii+1] = a2[(ii+1)*BLOCKSIZE] & 1;
			a_bits[ii+2] = a2[(ii+2)*BLOCKSIZE] & 1;
			a_bits[ii+3] = a2[(ii+3)*BLOCKSIZE] & 1;
		}

		#pragma omp single
		for (size_t ii = 4*(nextlevelblocks/8); ii < thislevelblocks; ii++) {
			if ((ii+1)*2 <= nextlevelblocks) {
				offline_prg(&b2[ii*2*expansion_stride], &a2[ii*BLOCKSIZE], bpo->keyL);
				offline_prg(&b2[(ii*2+1)*expansion_stride], &a2[ii*BLOCKSIZE], bpo->keyR);
			} else if (ii*2+1 <= nextlevelblocks) {
				offline_prg(&b2[ii*2*expansion_stride], &a2[ii*BLOCKSIZE], bpo->keyL);
			}
			a_bits[ii] = a2[ii*BLOCKSIZE] & 1;
		}

#ifdef ORAM_PROFILE_SCHEDULING
		#pragma omp single nowait
		printf("END FSS OFFLINE LEVEL 0 %lld\n", current_timestamp());
#endif

		for (thislevel = bpo->startlevel +1; thislevel < bpo->endlevel; thislevel++) {
			#pragma omp single
			omp_set_lock(&bpo->locks[thislevel- bpo->startlevel -1 ]);

#ifdef ORAM_PROFILE_SCHEDULING
			#pragma omp single nowait
			printf("START FSS OFFLINE LEVEL %d %lld\n", thislevel,current_timestamp());
#endif

			thislevelblocks = nextlevelblocks;
			nextlevelblocks = (bpo->size + (1ll<<(bpo->endlevel - thislevel -1)) - 1) / (1ll<<(bpo->endlevel - thislevel -1));
			if (thislevel == bpo->endlevel -1) nextlevelblocks = bpo->size;

			advicebit_l = bpo->advicebits_l[thislevel - bpo->startlevel -1];
			advicebit_r = bpo->advicebits_r[thislevel - bpo->startlevel -1];

			z = &((uint64_t *)bpo->Z)[(thislevel - bpo->startlevel -1) * (BLOCKSIZE/sizeof(uint64_t))];

			t2 = b2; t = b; t_bits = b_bits;
			b2 = a2; b = a; b_bits = a_bits;
			a2 = t2; a = t; a_bits = t_bits;

			if (thislevel == bpo->endlevel - 1 && b2 == local_output) {
				expansion_stride = (BLOCKSIZE * bpo->blockmultiple);
			} else {
				expansion_stride = BLOCKSIZE;
			}

			#pragma omp for schedule(guided)
			for (size_t ii = 0; ii < 4*(nextlevelblocks/8); ii+=4) {
				a_bits[ii] = (a2[ii*BLOCKSIZE] & 1) ^ (b_bits[ii/2] & advicebit_l);
				a_bits[ii+1] = (a2[(ii+1)*BLOCKSIZE] & 1) ^ (b_bits[ii/2] & advicebit_r);
				a_bits[ii+2] = (a2[(ii+2)*BLOCKSIZE] & 1) ^ (b_bits[(ii+2)/2] & advicebit_l);
				a_bits[ii+3] = (a2[(ii+3)*BLOCKSIZE] & 1) ^ (b_bits[(ii+2)/2] & advicebit_r);

				if (b_bits[ii/2]) {
					#pragma omp simd aligned(a,z:16)
					for (uint8_t jj = 0; jj < BLOCKSIZE/sizeof(uint64_t); jj++) {
						a[ii*(BLOCKSIZE/sizeof(uint64_t))+jj] ^= z[jj];
						a[(ii+1)*(BLOCKSIZE/sizeof(uint64_t))+jj] ^= z[jj];
					}
				}
				if (b_bits[(ii+2)/2]) {
					#pragma omp simd aligned(a,z:16)
					for (uint8_t jj = 0; jj < BLOCKSIZE/sizeof(uint64_t); jj++) {
						a[(ii+2)*(BLOCKSIZE/sizeof(uint64_t))+jj] ^= z[jj];
						a[(ii+3)*(BLOCKSIZE/sizeof(uint64_t))+jj] ^= z[jj];
					}
				}

				offline_prg_oct(&b2[ii*2*expansion_stride], &b2[(ii*2+1)*expansion_stride], &b2[(ii*2+2)*expansion_stride], &b2[(ii*2+3)*expansion_stride],
								&b2[(ii*2+4)*expansion_stride], &b2[(ii*2+5)*expansion_stride], &b2[(ii*2+6)*expansion_stride], &b2[(ii*2+7)*expansion_stride],
								&a2[ii*BLOCKSIZE],  &a2[ii*BLOCKSIZE], &a2[(ii+1)*BLOCKSIZE], &a2[(ii+1)*BLOCKSIZE],
								&a2[(ii+2)*BLOCKSIZE],  &a2[(ii+2)*BLOCKSIZE], &a2[(ii+3)*BLOCKSIZE], &a2[(ii+3)*BLOCKSIZE],
								bpo->keyL, bpo->keyR, bpo->keyL, bpo->keyR,
								bpo->keyL, bpo->keyR, bpo->keyL, bpo->keyR);
			}

			#pragma omp single
			for (size_t ii = 4*(nextlevelblocks/8); ii < thislevelblocks ; ii++) {

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
					offline_prg(&b2[ii*2*expansion_stride], &a2[ii*BLOCKSIZE], bpo->keyL);
					offline_prg(&b2[(ii*2+1)*expansion_stride], &a2[ii*BLOCKSIZE], bpo->keyR);
				} else if (ii*2+1 <= nextlevelblocks) {
					offline_prg(&b2[ii*2*expansion_stride], &a2[ii*BLOCKSIZE], bpo->keyL);
				}
			}

#ifdef ORAM_PROFILE_SCHEDULING
			#pragma omp single nowait
			printf("END FSS OFFLINE LEVEL %d %lld\n", thislevel,current_timestamp());
#endif
		}

		#pragma omp single
		omp_set_lock(&bpo->locks[thislevel- bpo->startlevel -1 ]);

#ifdef ORAM_PROFILE_SCHEDULING
		#pragma omp single nowait
		printf("START FSS LEVEL %d %lld\n", thislevel,current_timestamp());
#endif

		thislevelblocks = nextlevelblocks;

		advicebit_l = bpo->advicebits_l[thislevel - bpo->startlevel -1];
		advicebit_r = bpo->advicebits_r[thislevel - bpo->startlevel -1];

		z = &((uint64_t *)bpo->Z)[(thislevel - bpo->startlevel -1) * (BLOCKSIZE/sizeof(uint64_t))];

		t2 = b2; t = b; t_bits = b_bits;
		b2 = a2; b = a; b_bits = a_bits;
		a2 = t2; a = t; a_bits = t_bits;
		
		#pragma omp for schedule(guided)
		for (size_t ii = 0; ii < thislevelblocks; ii++) {
			if (b == local_output) {
				if (ii%2 == 0) {
					a_bits[ii] = (a2[ii*BLOCKSIZE] & 1) ^ (b_bits[ii/2] & advicebit_l);
				} else {
					a_bits[ii] = (a2[ii*BLOCKSIZE] & 1) ^ (b_bits[ii/2] & advicebit_r);
				}

				if (b_bits[ii/2]) {
					#pragma omp simd aligned(b,a,z:16)
					for (uint8_t jj = 0; jj < BLOCKSIZE/sizeof(uint64_t); jj++) {
						b[ii*((BLOCKSIZE * bpo->blockmultiple)/sizeof(uint64_t))+jj] = a[ii*(BLOCKSIZE/sizeof(uint64_t))+jj] ^ z[jj];
					}
				} else {
					memcpy(&b[ii*((BLOCKSIZE * bpo->blockmultiple)/sizeof(uint64_t))], &a[ii*(BLOCKSIZE/sizeof(uint64_t))], BLOCKSIZE);
				}
			} else {
				if (ii%2 == 0) {
					a_bits[ii] = (a2[ii*(BLOCKSIZE * bpo->blockmultiple)] & 1) ^ (b_bits[ii/2] & advicebit_l);
				} else {
					a_bits[ii] = (a2[ii*(BLOCKSIZE * bpo->blockmultiple)] & 1) ^ (b_bits[ii/2] & advicebit_r);
				}

				if (b_bits[ii/2]) {
					#pragma omp simd aligned(b,a,z:16)
					for (uint8_t jj = 0; jj < BLOCKSIZE/sizeof(uint64_t); jj++) {
						a[ii*((BLOCKSIZE * bpo->blockmultiple)/sizeof(uint64_t))+jj] ^= z[jj];
					}
				}
			}
		}

		#pragma omp single
		if (b == local_output) memcpy(b_bits, a_bits, thislevelblocks*sizeof(bool));

		if (bpo->blockmultiple > 1) {
			#pragma omp for schedule(guided)
			for (size_t ii = 0; ii < 8*(thislevelblocks/8); ii+=8)  {
				for (size_t jj = 1; jj < bpo->blockmultiple; jj++) {
					// Note to self: this is ridiculous. Define a macro.
					// Further note to self: actually, the non-encapsulation of offline_prg_oct and offline_prg
					// is just as ridiculous, if not more so. There has to be a better way. TODO: find it.
					offline_prg_oct(
						&local_output[(ii+0) * (BLOCKSIZE*bpo->blockmultiple) + (jj * BLOCKSIZE)],
						&local_output[(ii+1) * (BLOCKSIZE*bpo->blockmultiple) + (jj * BLOCKSIZE)],
						&local_output[(ii+2) * (BLOCKSIZE*bpo->blockmultiple) + (jj * BLOCKSIZE)],
						&local_output[(ii+3) * (BLOCKSIZE*bpo->blockmultiple) + (jj * BLOCKSIZE)],
						&local_output[(ii+4) * (BLOCKSIZE*bpo->blockmultiple) + (jj * BLOCKSIZE)],
						&local_output[(ii+5) * (BLOCKSIZE*bpo->blockmultiple) + (jj * BLOCKSIZE)],
						&local_output[(ii+6) * (BLOCKSIZE*bpo->blockmultiple) + (jj * BLOCKSIZE)],
						&local_output[(ii+7) * (BLOCKSIZE*bpo->blockmultiple) + (jj * BLOCKSIZE)],
						&local_output[(ii+0) * (BLOCKSIZE*bpo->blockmultiple) + ((jj-1) * BLOCKSIZE)],
						&local_output[(ii+1) * (BLOCKSIZE*bpo->blockmultiple) + ((jj-1) * BLOCKSIZE)],
						&local_output[(ii+2) * (BLOCKSIZE*bpo->blockmultiple) + ((jj-1) * BLOCKSIZE)],
						&local_output[(ii+3) * (BLOCKSIZE*bpo->blockmultiple) + ((jj-1) * BLOCKSIZE)],
						&local_output[(ii+4) * (BLOCKSIZE*bpo->blockmultiple) + ((jj-1) * BLOCKSIZE)],
						&local_output[(ii+5) * (BLOCKSIZE*bpo->blockmultiple) + ((jj-1) * BLOCKSIZE)],
						&local_output[(ii+6) * (BLOCKSIZE*bpo->blockmultiple) + ((jj-1) * BLOCKSIZE)],
						&local_output[(ii+7) * (BLOCKSIZE*bpo->blockmultiple) + ((jj-1) * BLOCKSIZE)],
						bpo->keyL,
						bpo->keyL,
						bpo->keyL,
						bpo->keyL,
						bpo->keyL,
						bpo->keyL,
						bpo->keyL,
						bpo->keyL
					);
				}
			}

			#pragma omp single
			for (size_t ii = 8*(thislevelblocks/8); ii < thislevelblocks ; ii++) {
				for (size_t jj = 1; jj < bpo->blockmultiple; jj++) {
					offline_prg(
						&local_output[(ii) * (BLOCKSIZE*bpo->blockmultiple) + (jj * BLOCKSIZE)],
						&local_output[(ii) * (BLOCKSIZE*bpo->blockmultiple) + ((jj-1) * BLOCKSIZE)],
						bpo->keyL
					);
				}
			}
		}

#ifdef ORAM_PROFILE_SCHEDULING
		#pragma omp single nowait
		printf("END FSS LEVEL %d %lld\n", thislevel,current_timestamp());
#endif
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
			{
				if (omp_get_num_threads() > 1) fn2(bp, bpo, pd);
				else fn2(bp, bpo, NULL);
			}			

			#pragma omp task
			bitpropagator_offline_readblockvector(local_output, local_bit_output, bpo);
		}
	}
}

bitpropagator_offline * bitpropagator_offline_new(size_t size, size_t blockmultiple, size_t startlevel, uint8_t * keyL, uint8_t * keyR) {
	bitpropagator_offline * bpo = malloc(sizeof(bitpropagator_offline));
	bpo->size = size;
	bpo->blockmultiple = blockmultiple;
	bpo->startlevel = startlevel;
	bpo->endlevel = LOG2LL(size) + (((1 << LOG2LL(size)) < size)? 1:0);
	posix_memalign(&bpo->level_data,16,(1ll<<bpo->endlevel) * BLOCKSIZE);
	posix_memalign(&bpo->Z,16,(bpo->endlevel - bpo->startlevel) * BLOCKSIZE);
	bpo->locks = malloc((bpo->endlevel - bpo->startlevel) * sizeof(omp_lock_t));
	bpo->advicebits_l = malloc((bpo->endlevel - bpo->startlevel) * sizeof(bool));
	bpo->advicebits_r = malloc((bpo->endlevel - bpo->startlevel) * sizeof(bool));
	bpo->level_bits = malloc(size * sizeof(bool));

	offline_prg_init();
	bpo->keyL = offline_prg_keyschedule(keyL);
	bpo->keyR = offline_prg_keyschedule(keyR); 

	for (int ii = 0; ii < (bpo->endlevel - bpo->startlevel); ii++) {
		omp_init_lock(&bpo->locks[ii]);
	}
	return bpo;
}

void bitpropagator_offline_free(bitpropagator_offline * bpo) {
	for (int ii = 0; ii < (bpo->endlevel - bpo->startlevel); ii++) {
		omp_destroy_lock(&bpo->locks[ii]);
	}
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