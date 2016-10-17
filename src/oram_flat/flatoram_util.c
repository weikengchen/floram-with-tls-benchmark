#include "flatoram_util.h"

#include <omp.h>

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

static int sslinits = 0;
static void* sslzero;
static omp_lock_t * ssllocks;

void get_random_bytes(void *buf, size_t bytes) {
	//only supported on recend linuxes, unfortunately.
	//getrandom(buf, bytes, 0);

	FILE *fp;
	fp = fopen("/dev/urandom", "r");
	fread(buf, 1, bytes, fp);
	fclose(fp);
}


// Locking callback
void openmp_locking_callback(int mode, int type, char *file, int line) {
	if (mode & CRYPTO_LOCK) {
		omp_set_lock(&ssllocks[type]);
	} else {
		omp_unset_lock(&ssllocks[type]);
	}
}

// Thread ID callback
unsigned long openmp_thread_id(void) {
	return (unsigned long)omp_get_thread_num();
}

void openmp_thread_setup(void) {
	ssllocks = OPENSSL_malloc(CRYPTO_num_locks() * sizeof(omp_lock_t));
	for (int ii=0; ii<CRYPTO_num_locks(); ii++) omp_init_lock(&ssllocks[ii]);
	CRYPTO_set_id_callback((unsigned long (*)())openmp_thread_id);
	CRYPTO_set_locking_callback((void (*)())openmp_locking_callback);
}

void openmp_thread_cleanup(void) {
	CRYPTO_set_id_callback(NULL);
	CRYPTO_set_locking_callback(NULL);
	for (int ii=0; ii<CRYPTO_num_locks(); ii++) omp_destroy_lock(&ssllocks[ii]);
	OPENSSL_free(ssllocks);
}



void offline_expand_init() {
	if (sslinits == 0) {
		openmp_thread_setup();
		ERR_load_crypto_strings();
		OpenSSL_add_all_algorithms();
		OPENSSL_config(NULL);
		sslzero = calloc(1, 16);
	}
	sslinits++;
}

void offline_expand_deinit() {
	if (sslinits == 1) {
		ENGINE_cleanup(); 
		CONF_modules_unload(1);
		EVP_cleanup();
		CRYPTO_cleanup_all_ex_data();
		ERR_remove_state(0);
		ERR_free_strings();
		free(sslzero);
		sslzero = NULL;
		openmp_thread_cleanup();
	}
	sslinits--;
}

void offline_expand(void * dest, void * src, size_t n) {
	EVP_CIPHER_CTX *ctx;
	ctx = EVP_CIPHER_CTX_new();
	EVP_EncryptInit_ex(ctx, EVP_aes_128_ctr(), NULL, src, sslzero);
	EVP_CIPHER_CTX_set_padding(ctx, 0);
	int len;
	for (size_t ii = 0; ii < n; ii++) {
		EVP_EncryptUpdate(ctx, &dest[BLOCKSIZE*ii], &len, sslzero, BLOCKSIZE);
	}
	EVP_CIPHER_CTX_free(ctx);
}