#include "floram_util.h"

#include <omp.h>
#include <unistd.h>

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

int floram_zpma(void** dst, size_t alignment, size_t size) {
   int res = posix_memalign(dst, alignment, size);
   memset(*dst, 0, size);
   return res;
}

uint32_t floram_atomic_read(uint32_t * x) {
    return __atomic_load_n(x, __ATOMIC_RELAXED);
}

void floram_atomic_inc(uint32_t * x) {
    return __atomic_fetch_add(x, 1, __ATOMIC_RELAXED);
}

int floram_usleep(uint64_t x) {
    return usleep(x);
}


void floram_set_procs_for_data_size(size_t dsize) {
#ifndef FLORAM_DISABLE_AUTO_THREAD_COUNT
    size_t recommended_cores = (dsize + CACHE_PER_CORE - 1) / CACHE_PER_CORE;
    size_t actual_cores = MIN(omp_get_num_procs(), MAX(1, recommended_cores));
    omp_set_num_threads(actual_cores);
#endif
}

#ifdef __AES__

#include <wmmintrin.h>
#include <tmmintrin.h>

#define KE(NK,OK,RND) NK = OK;	\
    NK = _mm_xor_si128(NK, _mm_slli_si128(NK, 4));	\
    NK = _mm_xor_si128(NK, _mm_slli_si128(NK, 4));	\
    NK = _mm_xor_si128(NK, _mm_slli_si128(NK, 4));	\
	OK = _mm_xor_si128(NK, _mm_shuffle_epi32(_mm_aeskeygenassist_si128(OK, RND), 0xff));

#define KE2(NK,OK,RND) NK = OK; \
    NK = _mm_xor_si128(NK, _mm_slli_si128(NK, 4));  \
    NK = _mm_xor_si128(NK, _mm_slli_si128(NK, 4));  \
    NK = _mm_xor_si128(NK, _mm_slli_si128(NK, 4));  \
    NK = _mm_xor_si128(NK, _mm_shuffle_epi32(_mm_aeskeygenassist_si128(OK, RND), 0xff));

void offline_prg_init() {
    // Do nothing
    return;
}

void * offline_prg_keyschedule(uint8_t * src) {
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

void offline_prg(uint8_t * dest, uint8_t * src, void * ri) {
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

void offline_prg_oct(uint8_t * dest1, uint8_t * dest2, uint8_t * dest3, uint8_t * dest4,
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

void offline_expand_from(uint8_t * dest, uint8_t * src, size_t i, size_t n) {
    // this version handles the case when n!=2 using a loop

    __m128i seed;
    seed = _mm_load_si128((__m128i *) src);

	__m128i r1,r2,r3,r4,r5,r6,r7,r8,r9,r10; // next key
	__m128i mr, ok;
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

    floram_set_procs_for_data_size(n*BLOCKSIZE);
    #pragma omp parallel for schedule(guided)
	for(size_t li=0; li<n-n%4; li+=4) {
        __m128i mr1, mr2, mr3, mr4;

		mr1 = _mm_set_epi64((__m64)(li+i),(__m64)0l);
		mr2 = _mm_set_epi64((__m64)(li+i+1),(__m64)0l);
		mr3 = _mm_set_epi64((__m64)(li+i+2),(__m64)0l);
		mr4 = _mm_set_epi64((__m64)(li+i+3),(__m64)0l);
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

	for(size_t li = n-n%4; li<n; li++) {
	    mr = _mm_set_epi64((__m64)(li+i),(__m64)0l);	// msg = li
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

#else //__AES__

#include "aes_gladman/aes.h"

#define htonll(x) ((1==htonl(1)) ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))

void offline_prg_init() {
    aes_init();
}

void * offline_prg_keyschedule(uint8_t * src) {
    uint8_t * r = malloc(11*16);

    aes_encrypt_ctx cx = {0};
    aes_encrypt_key128(src, &cx);
    memcpy(r, cx.ks, 11*16);
    return r;
}

void offline_prg(uint8_t * dest, uint8_t * src, void * ri) {
    aes_encrypt_ctx cx = {0};
    memcpy(cx.ks, ri, 11*16);
    cx.inf.l = 0;
    cx.inf.b[0] = 10 * 16;
    aes_encrypt(src, dest, &cx);
    #pragma omp simd
    for (uint8_t ii = 0; ii < 2; ii++) {
        ((uint64_t *) dest)[ii] ^= ((uint64_t *) src)[ii];
    }
}


void offline_prg_oct(uint8_t * dest1, uint8_t * dest2, uint8_t * dest3, uint8_t * dest4,
                        uint8_t * dest5, uint8_t * dest6, uint8_t * dest7, uint8_t * dest8, 
                        uint8_t * src1, uint8_t * src2, uint8_t * src3, uint8_t * src4,
                        uint8_t * src5, uint8_t * src6, uint8_t * src7, uint8_t * src8,
                        void * ri1, void * ri2 , void * ri3 , void * ri4,
                        void * ri5, void * ri6 , void * ri7 , void * ri8
                    ) {
    offline_prg(dest1, src1, ri1);
    offline_prg(dest2, src2, ri2);
    offline_prg(dest3, src3, ri3);
    offline_prg(dest4, src4, ri4);
    offline_prg(dest5, src5, ri5);
    offline_prg(dest6, src6, ri6);
    offline_prg(dest7, src7, ri7);
    offline_prg(dest8, src8, ri8);
}

void offline_expand_from(uint8_t * dest, uint8_t * src, size_t i, size_t n) {

    uint8_t * key = offline_prg_keyschedule(src);
    aes_encrypt_ctx cx = {0};
    memcpy(cx.ks, key, 11*16);
    cx.inf.l = 0;
    cx.inf.b[0] = 10 * 16;
    free(key);

    floram_set_procs_for_data_size(n*BLOCKSIZE);
    #pragma omp parallel for schedule(guided)
    for(size_t li=0; li<n; li++) {
        uint64_t iv[2] = {0,htonll(li+i)};
        aes_encrypt(iv,&dest[li*16],&cx);
    }
}

#endif //__AES__

void offline_expand(uint8_t * dest, uint8_t * src, size_t n) {
    offline_expand_from(dest, src, 0, n);
}