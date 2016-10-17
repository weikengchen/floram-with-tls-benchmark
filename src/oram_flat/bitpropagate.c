#include "bitpropagate.h"
#include "flatoram_util.h"

struct bitpropagator_offline {
	size_t size;
	uint32_t startlevel;
	uint32_t endlevel;
	uint32_t lastlevel;
	size_t blocksout;
	void * nextlevel_data;
	void * lastlevel_data;
};

void bitpropagator_offline_start(bitpropagator_offline * bpo, void * blocks) {
	bpo->lastlevel = bpo->startlevel;
	memcpy(bpo->nextlevel_data, blocks, (1<<bpo->startlevel) * BLOCKSIZE);

	size_t maxblocks = (bpo->size + (1<<(bpo->endlevel - bpo->lastlevel -1)) - 1) / (1<<(bpo->endlevel - bpo->lastlevel -1));
	#pragma omp parallel for
	for (size_t ii = 0; ii < maxblocks; ii += 2) {
		if (ii+2 <= maxblocks) {
			offline_expand(&bpo->lastlevel_data[ii*BLOCKSIZE], &bpo->nextlevel_data[ii/2*BLOCKSIZE], 2);
		} else {
			offline_expand(&bpo->lastlevel_data[ii*BLOCKSIZE], &bpo->nextlevel_data[ii/2*BLOCKSIZE], 1);
			ii += 1;
		}
	}
	bpo->blocksout = maxblocks;
}

void bitpropagator_offline_nextlevel(bitpropagator_offline * bpo, void * Z, uint32_t advicebit) {
	//First, apply the Z Block where appropriate.
	#pragma omp parallel for
	for (size_t ii = 0; ii < bpo->blocksout; ii++) {
		uint8_t abyte = advicebit/8;
		uint8_t abit = advicebit%8;
		if ((((uint8_t *)bpo->lastlevel_data)[ii*BLOCKSIZE + abyte] >> abit) & 1 == 1) {
			for (uint8_t jj = 0; jj < BLOCKSIZE/sizeof(uint64_t); jj++) {
				((uint64_t *)bpo->lastlevel_data)[ii*(BLOCKSIZE/sizeof(uint64_t)) + jj] ^= ((uint64_t *)Z)[jj];
			}
		}
	}
	bpo->lastlevel++;

	size_t maxblocks = (bpo->size + (1<<(bpo->endlevel - bpo->lastlevel -1)) - 1) / (1<<(bpo->endlevel - bpo->lastlevel -1));
	#pragma omp parallel for
	for (size_t ii = 0; ii < maxblocks; ii += 2) {
		if (ii+2 <= maxblocks) {
			offline_expand(&bpo->nextlevel_data[ii*BLOCKSIZE], &bpo->lastlevel_data[ii/2*BLOCKSIZE], 2);
		} else {
			offline_expand(&bpo->nextlevel_data[ii*BLOCKSIZE], &bpo->lastlevel_data[ii/2*BLOCKSIZE], 1);
		}
	}
	bpo->blocksout = maxblocks;
	memcpy(bpo->lastlevel_data, bpo->nextlevel_data, bpo->blocksout*BLOCKSIZE);
}

void bitpropagator_offline_lastlevel(bitpropagator_offline * bpo, void * Z, uint32_t advicebit) {
	#pragma omp parallel for
	for (size_t ii = 0; ii < bpo->blocksout; ii++) {
		uint8_t abyte = advicebit/8;
		uint8_t abit = advicebit%8;
		if ((((uint8_t *)bpo->lastlevel_data)[ii*BLOCKSIZE + abyte] >> abit) & 1 == 1) {
			for (uint8_t jj = 0; jj < BLOCKSIZE/sizeof(uint64_t); jj++) {
				((uint64_t *)bpo->lastlevel_data)[ii*(BLOCKSIZE/sizeof(uint64_t)) + jj] ^= ((uint64_t *)Z)[jj];
			}
		}
	}
	bpo->lastlevel++;
}

void bitpropagator_offline_readblockvector(void * local_output, bitpropagator_offline * bpo) {
	memcpy(local_output, bpo->lastlevel_data, bpo->size * BLOCKSIZE);
}

bitpropagator_offline * bitpropagator_offline_new(size_t size, uint32_t startlevel) {
	offline_expand_init();
	bitpropagator_offline * bpo = malloc(sizeof(bitpropagator_offline));
	bpo->size = size;
	bpo->startlevel = startlevel;
	bpo->endlevel = LOG2(size) + (((1 << LOG2(size)) < size)? 1:0);
	bpo->nextlevel_data = malloc((1<<bpo->endlevel) * BLOCKSIZE);
	bpo->lastlevel_data = malloc((1<<bpo->endlevel) * BLOCKSIZE);
	return bpo;
}

void bitpropagator_offline_free(bitpropagator_offline * bpo) {
	offline_expand_deinit();
	free(bpo->nextlevel_data);
	free(bpo->lastlevel_data);
	free(bpo);
}