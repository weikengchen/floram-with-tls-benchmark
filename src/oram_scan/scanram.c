#include "scanram.h"
#include <linux/random.h>

struct ramscan_context {

}

void get_random_bytes(void *buf, size_t bytes) {
	getrandom(buf, bytes, 0);
}

ramscan_context * ramscan_start(void * seed) {

}

void offline_expand(ramscan_context * rsc, void * dest, void * src, size_t n) {

}

void ramscan_add_branch_key(ramscan_context * rsc, void * key) {

}

void ramscan_result(void * data, ramscan_context * rsc) {

}

void ramscan_finish(ramscan_context * rsc) {

}