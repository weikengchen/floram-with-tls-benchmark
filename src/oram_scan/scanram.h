typedef struct ramscan_context ramscan_context;

void get_random_bytes(void *buf, size_t bytes);

ramscan_context * ramscan_start(void * seed);
void ramscan_add_branch_key(ramscan_context * rsc, void * key);
void ramscan_result(void * data, ramscan_context * rsc);
void ramscan_finish(ramscan_context * rsc);