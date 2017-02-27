#include "bitpropagate_cprg.h"
#include "floram_util.h"
#include "ackutil.h"
#include <omp.h>

struct bitpropagator_cprg_offline {
	size_t size;
	size_t blockmultiple;
	size_t startlevel;
	size_t thislevel;
	size_t endlevel;
	size_t thislevelblocks;
	size_t nextlevelblocks;
	void * Z;
	bool * advicebits_l;
	bool * advicebits_r;
	uint8_t * level_data;
	uint64_t * lda;
	uint64_t * ldb;
	uint8_t * lda2;
	uint8_t * ldb2;
	bool * level_bits;
	bool * lba;
	bool * lbb;
	void * keyL;
	void * keyR;
};

typedef struct block_t {
	uint64_t data[BLOCKSIZE/sizeof(uint64_t)];
} block_t;

void block_xor(block_t * a, block_t * b) {
	#pragma omp simd
	for (uint8_t ii = 0; ii < BLOCKSIZE/sizeof(uint64_t); ii++) {
		a->data[ii] ^= b->data[ii];
	}
}

#pragma omp declare reduction(^: block_t: block_xor(&omp_out, &omp_in)) initializer (omp_priv = { 0 })

void bitpropagator_cprg_offline_start(uint8_t * local_output, bool * local_bit_output, uint64_t * accumulator_L, uint64_t * accumulator_R, bitpropagator_cprg_offline * bpo) {
	bpo->thislevel = 0;
	bpo->thislevelblocks = 1;
	bpo->nextlevelblocks = 2;

	bpo->lda = (uint64_t *)bpo->level_data;
	bpo->lda2 = (uint8_t *)bpo->level_data;
	bpo->ldb = (uint64_t *)local_output;
	bpo->ldb2 = (uint8_t *)local_output;
	bpo->lba = bpo->level_bits;
	bpo->lbb = local_bit_output;

	memset(accumulator_L, 0, BLOCKSIZE);
	memset(accumulator_R, 0, BLOCKSIZE);
	get_random_bytes(bpo->lda2, BLOCKSIZE);
	if (ocCurrentParty() == 1) bpo->lda2[0] &= 0xFE;
	else bpo->lda2[0] |= 1;

	uint64_t * lda = bpo->lda;
	uint64_t * ldb = bpo->ldb;

	block_t accL;
	block_t accR;

	offline_prf(&bpo->ldb2[0], bpo->lda2, bpo->keyL);
	offline_prf(&bpo->ldb2[BLOCKSIZE], bpo->lda2, bpo->keyR);
	#pragma omp simd aligned(ldb, accumulator_L, accumulator_R:16) 
	for (size_t jj = 0; jj < BLOCKSIZE/sizeof(uint64_t); jj++) {
		accumulator_L[jj] ^= ldb[jj];
		accumulator_R[jj] ^= ldb[BLOCKSIZE/sizeof(uint64_t)+jj];
	}

	bpo->lba[0] = bpo->lda2[0] & 1;
}

void bitpropagator_cprg_offline_process_round(uint8_t * accumulator_L, uint8_t * accumulator_R, uint8_t * z, bool advicebit_l, bool advicebit_r, bitpropagator_cprg_offline * bpo) {
	bpo->thislevel += 1;
	bpo->thislevelblocks = bpo->nextlevelblocks;
	bpo->nextlevelblocks = (bpo->size + (1ll<<(bpo->endlevel - bpo->thislevel -1)) - 1) / (1ll<<(bpo->endlevel - bpo->thislevel -1));
	if (bpo->thislevel == bpo->endlevel -1) bpo->nextlevelblocks = bpo->size;

	uint64_t* t; uint8_t* t2; bool * tb;
	size_t expansion_stride;

	t2 = bpo->ldb2; t = bpo->ldb; tb = bpo->lbb;
	bpo->ldb2 = bpo->lda2; bpo->ldb = bpo->lda; bpo->lbb = bpo->lba;
	bpo->lda2 = t2; bpo->lda = t; bpo->lba = tb;

	uint64_t * lda = bpo->lda;
	uint64_t * ldb = bpo->ldb;

	block_t accL = {0};
	block_t accR = {0};

	if (bpo->thislevel == bpo->endlevel - 1 && (bpo->thislevel % 2) == 0) {
		expansion_stride = (BLOCKSIZE * bpo->blockmultiple);
	} else {
		expansion_stride = BLOCKSIZE;
	}

	#pragma omp parallel for reduction(^:accL,accR)
	for (size_t ii = 0; ii < 4*(bpo->nextlevelblocks/8); ii+=4) {
		bpo->lba[ii] = (bpo->lda2[ii*BLOCKSIZE] & 1) ^ (bpo->lbb[ii/2] & advicebit_l);
		bpo->lba[ii+1] = (bpo->lda2[(ii+1)*BLOCKSIZE] & 1) ^ (bpo->lbb[ii/2] & advicebit_r);
		bpo->lba[ii+2] = (bpo->lda2[(ii+2)*BLOCKSIZE] & 1) ^ (bpo->lbb[(ii+2)/2] & advicebit_l);
		bpo->lba[ii+3] = (bpo->lda2[(ii+3)*BLOCKSIZE] & 1) ^ (bpo->lbb[(ii+2)/2] & advicebit_r);

		if (bpo->lbb[ii/2]) {
			#pragma omp simd aligned(lda,z:16)
			for (uint8_t jj = 0; jj < BLOCKSIZE/sizeof(uint64_t); jj++) {
				lda[ii*(BLOCKSIZE/sizeof(uint64_t))+jj] ^= ((uint64_t *)z)[jj];
				lda[(ii+1)*(BLOCKSIZE/sizeof(uint64_t))+jj] ^= ((uint64_t *)z)[jj];
			}
		}

		if (bpo->lbb[(ii+2)/2]) {
			#pragma omp simd aligned(lda,z:16)
			for (uint8_t jj = 0; jj < BLOCKSIZE/sizeof(uint64_t); jj++) {
				lda[(ii+2)*(BLOCKSIZE/sizeof(uint64_t))+jj] ^= ((uint64_t *)z)[jj];
				lda[(ii+3)*(BLOCKSIZE/sizeof(uint64_t))+jj] ^= ((uint64_t *)z)[jj];
			}
		}

		offline_prf_oct(&bpo->ldb2[ii*2*expansion_stride], &bpo->ldb2[(ii*2+1)*expansion_stride], &bpo->ldb2[(ii*2+2)*expansion_stride], &bpo->ldb2[(ii*2+3)*expansion_stride],
						&bpo->ldb2[(ii*2+4)*expansion_stride], &bpo->ldb2[(ii*2+5)*expansion_stride], &bpo->ldb2[(ii*2+6)*expansion_stride], &bpo->ldb2[(ii*2+7)*expansion_stride],
						&bpo->lda2[ii*BLOCKSIZE],  &bpo->lda2[ii*BLOCKSIZE], &bpo->lda2[(ii+1)*BLOCKSIZE], &bpo->lda2[(ii+1)*BLOCKSIZE],
						&bpo->lda2[(ii+2)*BLOCKSIZE],  &bpo->lda2[(ii+2)*BLOCKSIZE], &bpo->lda2[(ii+3)*BLOCKSIZE], &bpo->lda2[(ii+3)*BLOCKSIZE],
						bpo->keyL, bpo->keyR, bpo->keyL, bpo->keyR,
						bpo->keyL, bpo->keyR, bpo->keyL, bpo->keyR);

		#pragma omp simd aligned(ldb:16)
		for (size_t jj = 0; jj < BLOCKSIZE/sizeof(uint64_t); jj++) {
			accL.data[jj] ^= ldb[ii*2*expansion_stride/sizeof(uint64_t)+jj] ^ ldb[(ii*2+2)*expansion_stride/sizeof(uint64_t)+jj]
								^ ldb[(ii*2+4)*expansion_stride/sizeof(uint64_t)+jj] ^ ldb[(ii*2+6)*expansion_stride/sizeof(uint64_t)+jj];
			accR.data[jj] ^= ldb[(ii*2+1)*expansion_stride/sizeof(uint64_t)+jj] ^ ldb[(ii*2+3)*expansion_stride/sizeof(uint64_t)+jj]
								^ ldb[(ii*2+5)*expansion_stride/sizeof(uint64_t)+jj] ^ ldb[(ii*2+7)*expansion_stride/sizeof(uint64_t)+jj];
		}
	}

	for (size_t ii = 4*(bpo->nextlevelblocks/8); ii < bpo->thislevelblocks ; ii++) {

		if (ii%2 == 0) {
			bpo->lba[ii] = (bpo->lda2[ii*BLOCKSIZE] & 1) ^ (bpo->lbb[ii/2] & advicebit_l);
		} else {
			bpo->lba[ii] = (bpo->lda2[ii*BLOCKSIZE] & 1) ^ (bpo->lbb[ii/2] & advicebit_r);
		}

		if (bpo->lbb[ii/2]) {
			#pragma omp simd aligned(lda,z:16)
			for (uint8_t jj = 0; jj < BLOCKSIZE/sizeof(uint64_t); jj++) {
				lda[ii*(BLOCKSIZE/sizeof(uint64_t))+jj] ^= ((uint64_t *)z)[jj];
			}
		}
	
		if ((ii+1)*2 <= bpo->nextlevelblocks) {
			offline_prf(&bpo->ldb2[ii*2*expansion_stride], &bpo->lda2[ii*BLOCKSIZE], bpo->keyL);
			offline_prf(&bpo->ldb2[(ii*2+1)*expansion_stride], &bpo->lda2[ii*BLOCKSIZE], bpo->keyR);
			#pragma omp simd aligned(ldb:16)
			for (size_t jj = 0; jj < BLOCKSIZE/sizeof(uint64_t); jj++) {
				accL.data[jj] ^= ldb[ii*2*expansion_stride/sizeof(uint64_t)+jj];
				accR.data[jj] ^= ldb[(ii*2+1)*expansion_stride/sizeof(uint64_t)+jj];
			}
		} else if (ii*2+1 <= bpo->nextlevelblocks) {
			offline_prf(&bpo->ldb2[ii*2*expansion_stride], &bpo->lda2[ii*BLOCKSIZE], bpo->keyL);
			#pragma omp simd aligned(ldb:16)
			for (size_t jj = 0; jj < BLOCKSIZE/sizeof(uint64_t); jj++) {
				accL.data[jj] ^= ldb[ii*2*expansion_stride/sizeof(uint64_t)+jj];
			}
		}
	}

	for (size_t jj = 0; jj < BLOCKSIZE/sizeof(uint64_t); jj++) {
		((uint64_t *)accumulator_L)[jj] = accL.data[jj];
		((uint64_t *)accumulator_R)[jj] = accR.data[jj];
	}
}

void bitpropagator_cprg_offline_finalize(uint8_t * accumulator, uint8_t * z, bool advicebit_l, bool advicebit_r, bitpropagator_cprg_offline * bpo) {
	bpo->thislevel += 1;
	bpo->thislevelblocks = bpo->nextlevelblocks;

	uint64_t* t; uint8_t* t2; bool * tb;

	t2 = bpo->ldb2; t = bpo->ldb; tb = bpo->lbb;
	bpo->ldb2 = bpo->lda2; bpo->ldb = bpo->lda; bpo->lbb = bpo->lba;
	bpo->lda2 = t2; bpo->lda = t; bpo->lba = tb;

	uint64_t * lda = bpo->lda;
	uint64_t * ldb = bpo->ldb;
	uint8_t * local_output;

	block_t acc = {0};

	if (bpo->thislevel%2==0) {
		local_output = bpo->ldb2;

		#pragma omp parallel for reduction(^:acc)
		for (size_t ii = 0; ii < bpo->thislevelblocks; ii++) {
			if (ii%2 == 0) {
				bpo->lba[ii] = (bpo->lda2[ii*BLOCKSIZE] & 1) ^ (bpo->lbb[ii/2] & advicebit_l);
			} else {
				bpo->lba[ii] = (bpo->lda2[ii*BLOCKSIZE] & 1) ^ (bpo->lbb[ii/2] & advicebit_r);
			}

			if (bpo->lbb[ii/2]) {
				#pragma omp simd aligned(ldb,lda,z:16)
				for (uint8_t jj = 0; jj < BLOCKSIZE/sizeof(uint64_t); jj++) {
					ldb[ii*((BLOCKSIZE*bpo->blockmultiple)/sizeof(uint64_t))+jj] = lda[ii*(BLOCKSIZE/sizeof(uint64_t))+jj] ^ ((uint64_t *)z)[jj];
					acc.data[jj] ^= ldb[ii*((BLOCKSIZE*bpo->blockmultiple)/sizeof(uint64_t))+jj];
				}
			} else {
				memcpy(&bpo->ldb[ii*((BLOCKSIZE*bpo->blockmultiple)/sizeof(uint64_t))], &bpo->lda[ii*(BLOCKSIZE/sizeof(uint64_t))], BLOCKSIZE);
				#pragma omp simd aligned(ldb:16)
				for (uint8_t jj = 0; jj < BLOCKSIZE/sizeof(uint64_t); jj++) {
					acc.data[jj] ^= ldb[ii*((BLOCKSIZE*bpo->blockmultiple)/sizeof(uint64_t))+jj];
				}
			}
		}

		memcpy(bpo->lbb, bpo->lba, bpo->thislevelblocks*sizeof(bool));

	} else {
		local_output = bpo->lda2;

		#pragma omp parallel for reduction(^:acc)
		for (size_t ii = 0; ii < bpo->thislevelblocks; ii++) {
			if (ii%2 == 0) {
				bpo->lba[ii] = (bpo->lda2[ii*(BLOCKSIZE*bpo->blockmultiple)] & 1) ^ (bpo->lbb[ii/2] & advicebit_l);
			} else {
				bpo->lba[ii] = (bpo->lda2[ii*(BLOCKSIZE*bpo->blockmultiple)] & 1) ^ (bpo->lbb[ii/2] & advicebit_r);
			}

			if (bpo->lbb[ii/2]) {
				#pragma omp simd aligned(lda,z:16)
				for (uint8_t jj = 0; jj < BLOCKSIZE/sizeof(uint64_t); jj++) {
					lda[ii*((BLOCKSIZE*bpo->blockmultiple)/sizeof(uint64_t))+jj] ^= ((uint64_t *)z)[jj];
					acc.data[jj] ^= lda[ii*((BLOCKSIZE*bpo->blockmultiple)/sizeof(uint64_t))+jj];
				}
			} else {
				#pragma omp simd aligned(lda:16)
				for (uint8_t jj = 0; jj < BLOCKSIZE/sizeof(uint64_t); jj++) {
					acc.data[jj] ^= lda[ii*((BLOCKSIZE*bpo->blockmultiple)/sizeof(uint64_t))+jj];
				}
			}
		}
	}

	for (size_t jj = 0; jj < BLOCKSIZE/sizeof(uint64_t); jj++) {
		((uint64_t *)accumulator)[jj] = acc.data[jj];
	}

	for (size_t jj = 1; jj < bpo->blockmultiple; jj++) {
		for (size_t ii = 0; ii < BLOCKSIZE/sizeof(uint64_t); ii++) acc.data[ii] = 0;

		#pragma omp parallel for reduction(^:acc)
		for (size_t ii = 0; ii < 8*(bpo->thislevelblocks/8); ii+=8)  {
			offline_prf_oct(
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
			#pragma omp simd aligned(local_output:16)
			for (size_t kk = 0; kk < BLOCKSIZE/sizeof(uint64_t); kk++) {
				acc.data[kk] ^= ((uint64_t *)(&local_output[(ii+0) * (BLOCKSIZE*bpo->blockmultiple) + (jj * BLOCKSIZE)]))[kk]
								^ ((uint64_t *)(&local_output[(ii+1) * (BLOCKSIZE*bpo->blockmultiple) + (jj * BLOCKSIZE)]))[kk]
								^ ((uint64_t *)(&local_output[(ii+2) * (BLOCKSIZE*bpo->blockmultiple) + (jj * BLOCKSIZE)]))[kk]
								^ ((uint64_t *)(&local_output[(ii+3) * (BLOCKSIZE*bpo->blockmultiple) + (jj * BLOCKSIZE)]))[kk]
								^ ((uint64_t *)(&local_output[(ii+4) * (BLOCKSIZE*bpo->blockmultiple) + (jj * BLOCKSIZE)]))[kk]
								^ ((uint64_t *)(&local_output[(ii+5) * (BLOCKSIZE*bpo->blockmultiple) + (jj * BLOCKSIZE)]))[kk]
								^ ((uint64_t *)(&local_output[(ii+6) * (BLOCKSIZE*bpo->blockmultiple) + (jj * BLOCKSIZE)]))[kk]
								^ ((uint64_t *)(&local_output[(ii+7) * (BLOCKSIZE*bpo->blockmultiple) + (jj * BLOCKSIZE)]))[kk];
			}
		}

		for (size_t ii = 8*(bpo->thislevelblocks/8); ii < bpo->thislevelblocks ; ii++) {
			offline_prf(
				&local_output[(ii) * (BLOCKSIZE*bpo->blockmultiple) + (jj * BLOCKSIZE)],
				&local_output[(ii) * (BLOCKSIZE*bpo->blockmultiple) + ((jj-1) * BLOCKSIZE)],
				bpo->keyL
			);

			for (size_t kk = 0; kk < BLOCKSIZE/sizeof(uint64_t); kk++) {
				acc.data[kk] ^= ((uint64_t *)(&local_output[(ii) * (BLOCKSIZE*bpo->blockmultiple) + (jj * BLOCKSIZE)]))[kk];
			}
		}

		for (size_t ii = 0; ii < BLOCKSIZE/sizeof(uint64_t); ii++) {
			((uint64_t *)accumulator)[jj*BLOCKSIZE/sizeof(uint64_t)+ii] = acc.data[ii];
		}
	}
}

void bitpropagator_cprg_offline_parallelizer(void* bp, void* indexp, void *blockdelta, void * local_output, void * local_bit_output, void* pd, bp_cprg_traverser_fn fn, facb_fn cbfn, void* cbpass) {

	omp_set_nested(true);

	#pragma omp parallel num_threads(2)
	{
		//OpenMP seems to get along with obliv-c just fine, so long as obliv-c only uses the master thread.
		#pragma omp master
		{
			fn(blockdelta, local_output, local_bit_output, bp, indexp);
		}
		
		if (*cbfn!=NULL) {
			#pragma omp single
			{
				#pragma omp task
				{
					if (omp_get_num_threads() > 1) cbfn(cbpass, pd);
					else cbfn(cbpass, NULL);
				}
			}
		}
	}
}

bitpropagator_cprg_offline * bitpropagator_cprg_offline_new(size_t size, size_t blockmultiple, uint8_t * keyL, uint8_t * keyR) {
	offline_expand_init();
	bitpropagator_cprg_offline * bpo = malloc(sizeof(bitpropagator_cprg_offline));
	bpo->size = size;
	bpo->blockmultiple = blockmultiple;
	bpo->startlevel = 0;
	bpo->endlevel = LOG2LL(size) + (((1 << LOG2LL(size)) < size)? 1:0);
	posix_memalign(&bpo->level_data,16,(1ll<<bpo->endlevel) * BLOCKSIZE);
	posix_memalign(&bpo->Z,16,(bpo->endlevel - bpo->startlevel) * BLOCKSIZE);
	bpo->advicebits_l = malloc((bpo->endlevel - bpo->startlevel) * sizeof(bool));
	bpo->advicebits_r = malloc((bpo->endlevel - bpo->startlevel) * sizeof(bool));
	bpo->level_bits = malloc(size * sizeof(bool));

	bpo->keyL = offline_prf_keyschedule(keyL);
	bpo->keyR = offline_prf_keyschedule(keyR); 

	return bpo;
}

void bitpropagator_cprg_offline_free(bitpropagator_cprg_offline * bpo) {
	offline_expand_deinit();
	free(bpo->level_data);
	free(bpo->level_bits);
	free(bpo->advicebits_l);
	free(bpo->advicebits_r);
	free(bpo->Z);
	free(bpo->keyL);
	free(bpo->keyR);
	free(bpo);
}