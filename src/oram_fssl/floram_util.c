#include "floram_util.h"

#include <omp.h>

#include <wmmintrin.h>
#include <tmmintrin.h>

void get_random_bytes(void *buf, size_t bytes) {
	//only supported on recent linuxes, unfortunately.
	//getrandom(buf, bytes, 0);

	FILE *fp = fopen("/dev/urandom", "r");
	if (fread(buf, 1, bytes, fp) != bytes) {
		fprintf(stderr,"Could not read random bytes.");
		exit(1);
	}
	fclose(fp);
}

int floram_pma(void** dst, size_t alignment, size_t size) {
	return posix_memalign(dst, alignment, size);
}

// Locking callback
void openmp_locking_callback(int mode, int type, char *file, int line) {
	//if (mode & CRYPTO_LOCK) {
	//	omp_set_lock(&ssllocks[type]);
	//} else {
	//	omp_unset_lock(&ssllocks[type]);
	//}
}

// Thread ID callback
unsigned long openmp_thread_id(void) {
	return (unsigned long)omp_get_thread_num();
}

void openmp_thread_setup(void) {
	//ssllocks = OPENSSL_malloc(CRYPTO_num_locks() * sizeof(omp_lock_t));
	//for (int ii=0; ii<CRYPTO_num_locks(); ii++) omp_init_lock(&ssllocks[ii]);
	//CRYPTO_set_id_callback((unsigned long (*)())openmp_thread_id);
	//CRYPTO_set_locking_callback((void (*)())openmp_locking_callback);
}

void openmp_thread_cleanup(void) {
	//CRYPTO_set_id_callback(NULL);
	//CRYPTO_set_locking_callback(NULL);
	//for (int ii=0; ii<CRYPTO_num_locks(); ii++) omp_destroy_lock(&ssllocks[ii]);
	//OPENSSL_free(ssllocks);
}

void offline_expand_init() {
	/*if (sslinits == 0) {
		openmp_thread_setup();
		ERR_load_crypto_strings();
		OpenSSL_add_all_algorithms();
		OPENSSL_config(NULL);
		sslzero = calloc(1, 16);
	}
	sslinits++;*/
}

void offline_expand_deinit() {
	/*if (sslinits == 1) {
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
	sslinits--;*/
}

#define KE(NK,OK,RND) NK = OK;	\
    NK = _mm_xor_si128(NK, _mm_slli_si128(NK, 4));	\
    NK = _mm_xor_si128(NK, _mm_slli_si128(NK, 4));	\
    NK = _mm_xor_si128(NK, _mm_slli_si128(NK, 4));	\
	OK = _mm_xor_si128(NK, _mm_shuffle_epi32(_mm_aeskeygenassist_si128(OK, RND), 0xff));


void offline_expand_2(uint8_t * dest, uint8_t * src) {

    __m128i seed;
    seed = _mm_load_si128((__m128i *) src);

	__m128i nk; // next key
	__m128i ml,mr, ok;
	ok = seed;

    ml = _mm_xor_si128(ml, ml); 				// msg = 0
    mr = _mm_set_epi64((__m64)0l,(__m64)1l);	// msg = 1

    // round 0
    ml = _mm_xor_si128(ml, ok);
    mr = _mm_xor_si128(mr, ok);

	// key expand 1 KEYEXP128(rk[0], 0x01);
	KE(nk, ok, 0x01)
    ml = _mm_aesenc_si128(ml, ok);
    mr = _mm_aesenc_si128(mr, ok);

	KE(nk, ok, 0x02)
    ml = _mm_aesenc_si128(ml, ok);
    mr = _mm_aesenc_si128(mr, ok);

	KE(nk, ok, 0x04)
    ml = _mm_aesenc_si128(ml, ok);
    mr = _mm_aesenc_si128(mr, ok);

	KE(nk, ok, 0x08)
    ml = _mm_aesenc_si128(ml, ok);
    mr = _mm_aesenc_si128(mr, ok);

	KE(nk, ok, 0x10)
    ml = _mm_aesenc_si128(ml, ok);
    mr = _mm_aesenc_si128(mr, ok);

	KE(nk, ok, 0x20)
    ml = _mm_aesenc_si128(ml, ok);
    mr = _mm_aesenc_si128(mr, ok);

	KE(nk, ok, 0x40)
    ml = _mm_aesenc_si128(ml, ok);
    mr = _mm_aesenc_si128(mr, ok);

	KE(nk, ok, 0x80)
    ml = _mm_aesenc_si128(ml, ok);
    mr = _mm_aesenc_si128(mr, ok);

	KE(nk, ok, 0x1b)
    ml = _mm_aesenc_si128(ml, ok);
    mr = _mm_aesenc_si128(mr, ok);

	KE(nk, ok, 0x36)
    ml = _mm_aesenclast_si128(ml, ok);
    mr = _mm_aesenclast_si128(mr, ok);

    _mm_storeu_si128((__m128i*) dest, ml);
    uint8_t* pp = (dest+16);
    _mm_storeu_si128((__m128i*) pp, mr);

}

void * offline_prf_keyschedule(uint8_t * src) {
#define KE2(NK,OK,RND) NK = OK;	\
    NK = _mm_xor_si128(NK, _mm_slli_si128(NK, 4));	\
    NK = _mm_xor_si128(NK, _mm_slli_si128(NK, 4));	\
    NK = _mm_xor_si128(NK, _mm_slli_si128(NK, 4));	\
	NK = _mm_xor_si128(NK, _mm_shuffle_epi32(_mm_aeskeygenassist_si128(OK, RND), 0xff));

	__m128i * r = malloc(11*sizeof(__m128i));

    r[0] = _mm_load_si128((__m128i *) src);

	KE2(r[1], r[0], 0x01)
	KE2(r[2], r[1], 0x02)
	KE2(r[3], r[2], 0x04)
	KE2(r[4], r[3], 0x08)
	KE2(r[5], r[4], 0x10)
	KE2(r[6], r[5], 0x20)
	KE2(r[7], r[6], 0x40)
	KE2(r[8], r[7], 0x80)
	KE2(r[9], r[8], 0x1b)
	KE2(r[10], r[9], 0x36)

	return r;
}

void offline_prf(uint8_t * dest, uint8_t * src, void * ri) {
	__m128i or, mr;
	__m128i * r = ri;

    or = _mm_load_si128((__m128i *) src);
    mr = or;

    mr = _mm_xor_si128(mr, r[0]);

    mr = _mm_aesenc_si128(mr, r[1]);
    mr = _mm_aesenc_si128(mr, r[2]);
    mr = _mm_aesenc_si128(mr, r[3]);
    mr = _mm_aesenc_si128(mr, r[4]);
    mr = _mm_aesenc_si128(mr, r[5]);
    mr = _mm_aesenc_si128(mr, r[6]);
    mr = _mm_aesenc_si128(mr, r[7]);
    mr = _mm_aesenc_si128(mr, r[8]);
    mr = _mm_aesenc_si128(mr, r[9]);
    mr = _mm_aesenclast_si128(mr, r[10]);
    mr = _mm_xor_si128(mr, or);
    _mm_storeu_si128((__m128i*) dest, mr);

}

void offline_prf_quad(uint8_t * dest1, uint8_t * dest2, uint8_t * dest3, uint8_t * dest4, uint8_t * src1, uint8_t * src2, uint8_t * src3, uint8_t * src4, void * ri1, void * ri2 , void * ri3 , void * ri4) {
	__m128i or1, or2, or3, or4, mr1, mr2, mr3, mr4;
	__m128i * r1 = ri1;
	__m128i * r2 = ri2;
	__m128i * r3 = ri3;
	__m128i * r4 = ri4;

    or1 = _mm_load_si128((__m128i *) src1);
    or2 = _mm_load_si128((__m128i *) src2);
    or3 = _mm_load_si128((__m128i *) src3);
    or4 = _mm_load_si128((__m128i *) src4);

    mr1 = or1;
    mr2 = or2;
    mr3 = or3;
    mr4 = or4;

    mr1 = _mm_xor_si128(mr1, r1[0]);
    mr2 = _mm_xor_si128(mr2, r2[0]);
    mr3 = _mm_xor_si128(mr3, r3[0]);
    mr4 = _mm_xor_si128(mr4, r4[0]);

    mr1 = _mm_aesenc_si128(mr1, r1[1]);
    mr2 = _mm_aesenc_si128(mr2, r2[1]);
    mr3 = _mm_aesenc_si128(mr3, r3[1]);
    mr4 = _mm_aesenc_si128(mr4, r4[1]);

    mr1 = _mm_aesenc_si128(mr1, r1[2]);
    mr2 = _mm_aesenc_si128(mr2, r2[2]);
    mr3 = _mm_aesenc_si128(mr3, r3[2]);
    mr4 = _mm_aesenc_si128(mr4, r4[2]);

    mr1 = _mm_aesenc_si128(mr1, r1[3]);
    mr2 = _mm_aesenc_si128(mr2, r2[3]);
    mr3 = _mm_aesenc_si128(mr3, r3[3]);
    mr4 = _mm_aesenc_si128(mr4, r4[3]);

    mr1 = _mm_aesenc_si128(mr1, r1[4]);
    mr2 = _mm_aesenc_si128(mr2, r2[4]);
    mr3 = _mm_aesenc_si128(mr3, r3[4]);
    mr4 = _mm_aesenc_si128(mr4, r4[4]);

    mr1 = _mm_aesenc_si128(mr1, r1[5]);
    mr2 = _mm_aesenc_si128(mr2, r2[5]);
    mr3 = _mm_aesenc_si128(mr3, r3[5]);
    mr4 = _mm_aesenc_si128(mr4, r4[5]);

    mr1 = _mm_aesenc_si128(mr1, r1[6]);
    mr2 = _mm_aesenc_si128(mr2, r2[6]);
    mr3 = _mm_aesenc_si128(mr3, r3[6]);
    mr4 = _mm_aesenc_si128(mr4, r4[6]);

    mr1 = _mm_aesenc_si128(mr1, r1[7]);
    mr2 = _mm_aesenc_si128(mr2, r2[7]);
    mr3 = _mm_aesenc_si128(mr3, r3[7]);
    mr4 = _mm_aesenc_si128(mr4, r4[7]);

    mr1 = _mm_aesenc_si128(mr1, r1[8]);
    mr2 = _mm_aesenc_si128(mr2, r2[8]);
    mr3 = _mm_aesenc_si128(mr3, r3[8]);
    mr4 = _mm_aesenc_si128(mr4, r4[8]);

    mr1 = _mm_aesenc_si128(mr1, r1[9]);
    mr2 = _mm_aesenc_si128(mr2, r2[9]);
    mr3 = _mm_aesenc_si128(mr3, r3[9]);
    mr4 = _mm_aesenc_si128(mr4, r4[9]);

    mr1 = _mm_aesenclast_si128(mr1, r1[10]);
    mr2 = _mm_aesenclast_si128(mr2, r2[10]);
    mr3 = _mm_aesenclast_si128(mr3, r3[10]);
    mr4 = _mm_aesenclast_si128(mr4, r4[10]);

    mr1 = _mm_xor_si128(mr1, or1);
    mr2 = _mm_xor_si128(mr2, or2);
    mr3 = _mm_xor_si128(mr3, or3);
    mr4 = _mm_xor_si128(mr4, or4);

    _mm_storeu_si128((__m128i*) dest1, mr1);
    _mm_storeu_si128((__m128i*) dest2, mr2);
    _mm_storeu_si128((__m128i*) dest3, mr3);
    _mm_storeu_si128((__m128i*) dest4, mr4);
}

void offline_prf_oct(uint8_t * dest1, uint8_t * dest2, uint8_t * dest3, uint8_t * dest4,
						uint8_t * dest5, uint8_t * dest6, uint8_t * dest7, uint8_t * dest8, 
						uint8_t * src1, uint8_t * src2, uint8_t * src3, uint8_t * src4,
						uint8_t * src5, uint8_t * src6, uint8_t * src7, uint8_t * src8,
						void * ri1, void * ri2 , void * ri3 , void * ri4,
						void * ri5, void * ri6 , void * ri7 , void * ri8
					) {
	__m128i * mr1 = dest1;
    __m128i * mr2 = dest2;
    __m128i * mr3 = dest3;
    __m128i * mr4 = dest4;
    __m128i * mr5 = dest5;
    __m128i * mr6 = dest6;
    __m128i * mr7 = dest7;
    __m128i * mr8 = dest8;
	__m128i * r1 = ri1;
	__m128i * r2 = ri2;
	__m128i * r3 = ri3;
	__m128i * r4 = ri4;
	__m128i * r5 = ri5;
	__m128i * r6 = ri6;
	__m128i * r7 = ri7;
	__m128i * r8 = ri8;

    *mr1 = _mm_load_si128((__m128i *) src1);
    *mr2 = _mm_load_si128((__m128i *) src2);
    *mr3 = _mm_load_si128((__m128i *) src3);
    *mr4 = _mm_load_si128((__m128i *) src4);
    *mr5 = _mm_load_si128((__m128i *) src5);
    *mr6 = _mm_load_si128((__m128i *) src6);
    *mr7 = _mm_load_si128((__m128i *) src7);
    *mr8 = _mm_load_si128((__m128i *) src8);

    *mr1 = _mm_xor_si128(*mr1, r1[0]);
    *mr2 = _mm_xor_si128(*mr2, r2[0]);
    *mr3 = _mm_xor_si128(*mr3, r3[0]);
    *mr4 = _mm_xor_si128(*mr4, r4[0]);
    *mr5 = _mm_xor_si128(*mr5, r5[0]);
    *mr6 = _mm_xor_si128(*mr6, r6[0]);
    *mr7 = _mm_xor_si128(*mr7, r7[0]);
    *mr8 = _mm_xor_si128(*mr8, r8[0]);

    *mr1 = _mm_aesenc_si128(*mr1, r1[1]);
    *mr2 = _mm_aesenc_si128(*mr2, r2[1]);
    *mr3 = _mm_aesenc_si128(*mr3, r3[1]);
    *mr4 = _mm_aesenc_si128(*mr4, r4[1]);
    *mr5 = _mm_aesenc_si128(*mr5, r5[1]);
    *mr6 = _mm_aesenc_si128(*mr6, r6[1]);
    *mr7 = _mm_aesenc_si128(*mr7, r7[1]);
    *mr8 = _mm_aesenc_si128(*mr8, r8[1]);

    *mr1 = _mm_aesenc_si128(*mr1, r1[2]);
    *mr2 = _mm_aesenc_si128(*mr2, r2[2]);
    *mr3 = _mm_aesenc_si128(*mr3, r3[2]);
    *mr4 = _mm_aesenc_si128(*mr4, r4[2]);
    *mr5 = _mm_aesenc_si128(*mr5, r5[2]);
    *mr6 = _mm_aesenc_si128(*mr6, r6[2]);
    *mr7 = _mm_aesenc_si128(*mr7, r7[2]);
    *mr8 = _mm_aesenc_si128(*mr8, r8[2]);

    *mr1 = _mm_aesenc_si128(*mr1, r1[3]);
    *mr2 = _mm_aesenc_si128(*mr2, r2[3]);
    *mr3 = _mm_aesenc_si128(*mr3, r3[3]);
    *mr4 = _mm_aesenc_si128(*mr4, r4[3]);
    *mr5 = _mm_aesenc_si128(*mr5, r5[3]);
    *mr6 = _mm_aesenc_si128(*mr6, r6[3]);
    *mr7 = _mm_aesenc_si128(*mr7, r7[3]);
    *mr8 = _mm_aesenc_si128(*mr8, r8[3]);

    *mr1 = _mm_aesenc_si128(*mr1, r1[4]);
    *mr2 = _mm_aesenc_si128(*mr2, r2[4]);
    *mr3 = _mm_aesenc_si128(*mr3, r3[4]);
    *mr4 = _mm_aesenc_si128(*mr4, r4[4]);
    *mr5 = _mm_aesenc_si128(*mr5, r5[4]);
    *mr6 = _mm_aesenc_si128(*mr6, r6[4]);
    *mr7 = _mm_aesenc_si128(*mr7, r7[4]);
    *mr8 = _mm_aesenc_si128(*mr8, r8[4]);

    *mr1 = _mm_aesenc_si128(*mr1, r1[5]);
    *mr2 = _mm_aesenc_si128(*mr2, r2[5]);
    *mr3 = _mm_aesenc_si128(*mr3, r3[5]);
    *mr4 = _mm_aesenc_si128(*mr4, r4[5]);
    *mr5 = _mm_aesenc_si128(*mr5, r5[5]);
    *mr6 = _mm_aesenc_si128(*mr6, r6[5]);
    *mr7 = _mm_aesenc_si128(*mr7, r7[5]);
    *mr8 = _mm_aesenc_si128(*mr8, r8[5]);

    *mr1 = _mm_aesenc_si128(*mr1, r1[6]);
    *mr2 = _mm_aesenc_si128(*mr2, r2[6]);
    *mr3 = _mm_aesenc_si128(*mr3, r3[6]);
    *mr4 = _mm_aesenc_si128(*mr4, r4[6]);
    *mr5 = _mm_aesenc_si128(*mr5, r5[6]);
    *mr6 = _mm_aesenc_si128(*mr6, r6[6]);
    *mr7 = _mm_aesenc_si128(*mr7, r7[6]);
    *mr8 = _mm_aesenc_si128(*mr8, r8[6]);

    *mr1 = _mm_aesenc_si128(*mr1, r1[7]);
    *mr2 = _mm_aesenc_si128(*mr2, r2[7]);
    *mr3 = _mm_aesenc_si128(*mr3, r3[7]);
    *mr4 = _mm_aesenc_si128(*mr4, r4[7]);
    *mr5 = _mm_aesenc_si128(*mr5, r5[7]);
    *mr6 = _mm_aesenc_si128(*mr6, r6[7]);
    *mr7 = _mm_aesenc_si128(*mr7, r7[7]);
    *mr8 = _mm_aesenc_si128(*mr8, r8[7]);

    *mr1 = _mm_aesenc_si128(*mr1, r1[8]);
    *mr2 = _mm_aesenc_si128(*mr2, r2[8]);
    *mr3 = _mm_aesenc_si128(*mr3, r3[8]);
    *mr4 = _mm_aesenc_si128(*mr4, r4[8]);
    *mr5 = _mm_aesenc_si128(*mr5, r5[8]);
    *mr6 = _mm_aesenc_si128(*mr6, r6[8]);
    *mr7 = _mm_aesenc_si128(*mr7, r7[8]);
    *mr8 = _mm_aesenc_si128(*mr8, r8[8]);

    *mr1 = _mm_aesenc_si128(*mr1, r1[9]);
    *mr2 = _mm_aesenc_si128(*mr2, r2[9]);
    *mr3 = _mm_aesenc_si128(*mr3, r3[9]);
    *mr4 = _mm_aesenc_si128(*mr4, r4[9]);
    *mr5 = _mm_aesenc_si128(*mr5, r5[9]);
    *mr6 = _mm_aesenc_si128(*mr6, r6[9]);
    *mr7 = _mm_aesenc_si128(*mr7, r7[9]);
    *mr8 = _mm_aesenc_si128(*mr8, r8[9]);

    *mr1 = _mm_aesenclast_si128(*mr1, r1[10]);
    *mr2 = _mm_aesenclast_si128(*mr2, r2[10]);
    *mr3 = _mm_aesenclast_si128(*mr3, r3[10]);
    *mr4 = _mm_aesenclast_si128(*mr4, r4[10]);
    *mr5 = _mm_aesenclast_si128(*mr5, r5[10]);
    *mr6 = _mm_aesenclast_si128(*mr6, r6[10]);
    *mr7 = _mm_aesenclast_si128(*mr7, r7[10]);
    *mr8 = _mm_aesenclast_si128(*mr8, r8[10]);

    *mr1 = _mm_xor_si128(*mr1, _mm_load_si128((__m128i *) src1));
    *mr2 = _mm_xor_si128(*mr2, _mm_load_si128((__m128i *) src2));
    *mr3 = _mm_xor_si128(*mr3, _mm_load_si128((__m128i *) src3));
    *mr4 = _mm_xor_si128(*mr4, _mm_load_si128((__m128i *) src4));
    *mr5 = _mm_xor_si128(*mr5, _mm_load_si128((__m128i *) src5));
    *mr6 = _mm_xor_si128(*mr6, _mm_load_si128((__m128i *) src6));
    *mr7 = _mm_xor_si128(*mr7, _mm_load_si128((__m128i *) src7));
    *mr8 = _mm_xor_si128(*mr8, _mm_load_si128((__m128i *) src8));
}

void offline_expand(uint8_t * dest, uint8_t * src, size_t n) {
	offline_expand_from(dest, src, 0, n);
}

void offline_expand_from(uint8_t * dest, uint8_t * src, size_t i, size_t n) {
#define KE2(NK,OK,RND) NK = OK;	\
    NK = _mm_xor_si128(NK, _mm_slli_si128(NK, 4));	\
    NK = _mm_xor_si128(NK, _mm_slli_si128(NK, 4));	\
    NK = _mm_xor_si128(NK, _mm_slli_si128(NK, 4));	\
	NK = _mm_xor_si128(NK, _mm_shuffle_epi32(_mm_aeskeygenassist_si128(OK, RND), 0xff));

    // this version handles the case when n!=2 using a loop

    __m128i seed;
    seed = _mm_load_si128((__m128i *) src);

	__m128i r1,r2,r3,r4,r5,r6,r7,r8,r9,r10; // next key
	__m128i mr, mr1, mr2, mr3, mr4, ok;
	ok = seed;

	KE2(r1, ok, 0x01)
	KE2(r2, r1, 0x02)
	KE2(r3, r2, 0x04)
	KE2(r4, r3, 0x08)
	KE2(r5, r4, 0x10)
	KE2(r6, r5, 0x20)
	KE2(r7, r6, 0x40)
	KE2(r8, r7, 0x80)
	KE2(r9, r8, 0x1b)
	KE2(r10, r9, 0x36)

	__m128i mask = _mm_set_epi64((__m64)0x08090a0b0c0d0e0fULL, (__m64)0x0001020304050607ULL );

	size_t li=i;

	for(; li<n-n%4; li+=4) {
		mr1 = _mm_set_epi64((__m64)li,(__m64)0l);
		mr2 = _mm_set_epi64((__m64)li+1,(__m64)0l);
		mr3 = _mm_set_epi64((__m64)li+2,(__m64)0l);
		mr4 = _mm_set_epi64((__m64)li+3,(__m64)0l);
		mr1 = _mm_shuffle_epi8 (mr1, mask);
		mr2 = _mm_shuffle_epi8 (mr2, mask);
		mr3 = _mm_shuffle_epi8 (mr3, mask);
		mr4 = _mm_shuffle_epi8 (mr4, mask);

	    mr1 = _mm_xor_si128(mr1, ok);
	    mr2 = _mm_xor_si128(mr2, ok);
	    mr3 = _mm_xor_si128(mr3, ok);
	    mr4 = _mm_xor_si128(mr4, ok);

	    mr1 = _mm_aesenc_si128(mr1, r1);
	    mr2 = _mm_aesenc_si128(mr2, r1);
	    mr3 = _mm_aesenc_si128(mr3, r1);
	    mr4 = _mm_aesenc_si128(mr4, r1);

	    mr1 = _mm_aesenc_si128(mr1, r2);
	    mr2 = _mm_aesenc_si128(mr2, r2);
	    mr3 = _mm_aesenc_si128(mr3, r2);
	    mr4 = _mm_aesenc_si128(mr4, r2);

	    mr1 = _mm_aesenc_si128(mr1, r3);
	    mr2 = _mm_aesenc_si128(mr2, r3);
	    mr3 = _mm_aesenc_si128(mr3, r3);
	    mr4 = _mm_aesenc_si128(mr4, r3);

	    mr1 = _mm_aesenc_si128(mr1, r4);
	    mr2 = _mm_aesenc_si128(mr2, r4);
	    mr3 = _mm_aesenc_si128(mr3, r4);
	    mr4 = _mm_aesenc_si128(mr4, r4);

	    mr1 = _mm_aesenc_si128(mr1, r5);
	    mr2 = _mm_aesenc_si128(mr2, r5);
	    mr3 = _mm_aesenc_si128(mr3, r5);
	    mr4 = _mm_aesenc_si128(mr4, r5);

	    mr1 = _mm_aesenc_si128(mr1, r6);
	    mr2 = _mm_aesenc_si128(mr2, r6);
	    mr3 = _mm_aesenc_si128(mr3, r6);
	    mr4 = _mm_aesenc_si128(mr4, r6);

	    mr1 = _mm_aesenc_si128(mr1, r7);
	    mr2 = _mm_aesenc_si128(mr2, r7);
	    mr3 = _mm_aesenc_si128(mr3, r7);
	    mr4 = _mm_aesenc_si128(mr4, r7);

	    mr1 = _mm_aesenc_si128(mr1, r8);
	    mr2 = _mm_aesenc_si128(mr2, r8);
	    mr3 = _mm_aesenc_si128(mr3, r8);
	    mr4 = _mm_aesenc_si128(mr4, r8);

	    mr1 = _mm_aesenc_si128(mr1, r9);
	    mr2 = _mm_aesenc_si128(mr2, r9);
	    mr3 = _mm_aesenc_si128(mr3, r9);
	    mr4 = _mm_aesenc_si128(mr4, r9);

	    mr1 = _mm_aesenclast_si128(mr1, r10);
	    mr2 = _mm_aesenclast_si128(mr2, r10);
	    mr3 = _mm_aesenclast_si128(mr3, r10);
	    mr4 = _mm_aesenclast_si128(mr4, r10);

	    uint8_t* pp1 = dest+(li*16);
	    uint8_t* pp2 = dest+((li+1)*16);
	    uint8_t* pp3 = dest+((li+2)*16);
	    uint8_t* pp4 = dest+((li+3)*16);

	    _mm_storeu_si128((__m128i*) pp1, mr1);
	    _mm_storeu_si128((__m128i*) pp2, mr2);
	    _mm_storeu_si128((__m128i*) pp3, mr3);
	    _mm_storeu_si128((__m128i*) pp4, mr4);
	}

	for(; li<n; li++) {
	    mr = _mm_set_epi64((__m64)li,(__m64)0l);	// msg = li
		mr = _mm_shuffle_epi8 (mr, mask);

	    mr = _mm_xor_si128(mr, ok);					// round 0

	    mr = _mm_aesenc_si128(mr, r1);
	    mr = _mm_aesenc_si128(mr, r2);
	    mr = _mm_aesenc_si128(mr, r3);
	    mr = _mm_aesenc_si128(mr, r4);
	    mr = _mm_aesenc_si128(mr, r5);
	    mr = _mm_aesenc_si128(mr, r6);
	    mr = _mm_aesenc_si128(mr, r7);
	    mr = _mm_aesenc_si128(mr, r8);
	    mr = _mm_aesenc_si128(mr, r9);
	    mr = _mm_aesenclast_si128(mr, r10);
	    uint8_t* pp = dest+(li*16);
	    _mm_storeu_si128((__m128i*) pp, mr);

	}



}

