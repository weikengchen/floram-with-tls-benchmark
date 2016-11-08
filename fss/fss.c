// how to compile : gcc -I/usr/local/Cellar/openssl/1.0.2h_1/include  -L/usr/local/Cellar/openssl/1.0.2h_1/lib -maes -o fss fss.c -lssl -lcrypto

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <wmmintrin.h>
#include <stdint.h>
#include <openssl/aes.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>


#ifdef __MACH__
#include <sys/time.h>
#define CLOCK_PROCESS_CPUTIME_ID 0
//clock_gettime is not implemented on OSX
int clock_gettime(int clk_id, struct timespec* t) {
    struct timeval now;
    int rv = gettimeofday(&now, NULL);
    if (rv) return rv;
    t->tv_sec  = now.tv_sec;
    t->tv_nsec = now.tv_usec;
    return 0;
}
#endif


struct timespec timer_start(){
    struct timespec start_time;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_time);
    return start_time;
}

// call this function to end a timer, returning nanoseconds elapsed as a long
long timer_end(struct timespec start_time){
    struct timespec end_time;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end_time);
    long diffInNanos = 
    	(end_time.tv_sec - start_time.tv_sec)*1000000+
    	end_time.tv_nsec - start_time.tv_nsec;
    return diffInNanos;
}


#define KE(NK,OK,RND) NK = OK;	\
    NK = _mm_xor_si128(NK, _mm_slli_si128(NK, 4));	\
    NK = _mm_xor_si128(NK, _mm_slli_si128(NK, 4));	\
    NK = _mm_xor_si128(NK, _mm_slli_si128(NK, 4));	\
	OK = _mm_xor_si128(NK, _mm_shuffle_epi32(_mm_aeskeygenassist_si128(OK, RND), 0xff)); \


// takes one seed (using last bit as its flag), and expands to two child seeds and bits
// this is the only place where crypto (the PRG) is used, replace aes here with any PRG
void fss_expand(const unsigned char* s0, unsigned char* outl, unsigned char* outr) {

    __m128i seed;
    seed = _mm_load_si128((__m128i *) s0);

	__m128i nk; // next key
	__m128i ok; // next key
	__m128i ml,mr;
	ok = seed;

    ml = _mm_xor_si128(ml, ml); 	// msg = 0
    mr = _mm_set_epi64((__m64)0l,(__m64)1l);
    //m1 = _mm_xor_si128(ml, 0x000000001); 	// msg = 0

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
    mr = _mm_aesenc_si128(mr, ok);

    _mm_storeu_si128((__m128i*) outl, ml);
    _mm_storeu_si128((__m128i*) outr, mr);


}


// computes fresh fss shares for the distributed point function f_{index,value} 
//     i.e. f(index) = value, f(not-index) = 0
//     returns a key structure for the parties in k0[], k1[], and returns the flag bit advice in t[]
void fss_dpf_compute_key(int index, int value, int depth, uint64_t* k0, uint64_t* k1, uint8_t* t) {

	int ai, keep, leave;

	// read random bytes as seeds
	int rnd = open("/dev/random", O_RDONLY);
	if (!rnd) { exit(1); }
	if ( read(rnd, k0, 16)!=16 || read(rnd,k1,16)!=16 ) {
		fprintf(stderr,"couldnt read from rand\n"); 
		exit(1);
	}
	close(rnd);

	uint64_t tt1[2] __attribute__ ((aligned (16))) = { 0x10203ff050607080, 0x90a0b0c0d0e0f000 };
	uint64_t tt2[2] __attribute__ ((aligned (16))) = { 0x10203ff050ffffff, 0x90a0b0c0d0e0ffff };

	k0[0]=tt1[0]; k0[1]=tt1[0];
	k1[0]=tt2[0]; k1[1]=tt2[0];

	// enforce seeds begins with opposite t bits
	uint8_t t0 = k0[0]&0x1;
	uint8_t t1 = k1[0]&0x1;
	if (t0==t1) {
		k1[0]=k1[0]^0x1;
		t1 = k1[0]&0x1;
	}

	uint64_t s0[2] __attribute__ ((aligned (16)));
	uint64_t s1[2] __attribute__ ((aligned (16)));

	uint64_t s0l[2] __attribute__ ((aligned (16)));
	uint64_t s0r[2] __attribute__ ((aligned (16)));
	uint64_t s1l[2] __attribute__ ((aligned (16)));
	uint64_t s1r[2] __attribute__ ((aligned (16)));


	s0[0] = k0[0]; s0[1] = k0[1];
	s1[0] = k1[0]; s1[1] = k1[1];

	printf(" ======== f_{%d, %d} ======== \n",index, value);

	for (int i=1; i<=depth; i++) {
		ai = (index >> (depth-i)) & 0x1;

		// set low order bits to zero. TODO: is this necessary?
		//s0[0] ^= t0;
		//s1[0] ^= t1;

		printf("a_{%d} = %d ------------------\n", i, ai);

		fss_expand((unsigned char*)s0, (unsigned char*)s0l, (unsigned char*)s0r);
		fss_expand((unsigned char*)s1, (unsigned char*)s1l, (unsigned char*)s1r);

		printf("  s0: %016llx%016llx|%d\n", s0[1],s0[0], t0);
		printf("      %.16llx%.16llx  %.16llx%.16llx\n", s0l[1],s0l[0], s0r[1],s0r[0]); 
		printf("  s1: %016llx%016llx|%d\n", s1[1],s1[0], t1);
		printf("      %.16llx%.16llx  %.16llx%.16llx\n",  			
			s1l[1],s1l[0], s1r[1],s1r[0]);

		uint8_t ti0, ti1, tcw;
		uint8_t tlcw = (s0l[0]&0x1) ^ (s1l[0]&01) ^ ai ^ 1;
		uint8_t trcw = (s0r[0]&0x1) ^ (s1r[0]&01) ^ ai;

		__m128i v0l,v1l,v0r,v1r, scw, sk0, sk1;
	    v0l = _mm_load_si128((__m128i *) s0l);
	    v1l = _mm_load_si128((__m128i *) s1l);
	    v0r = _mm_load_si128((__m128i *) s0r);
	    v1r = _mm_load_si128((__m128i *) s1r);


		if (ai) {	// ai=1, keep=rightpath
		    scw = _mm_xor_si128(v0l, v1l);

		    ti0 = s0r[0] & 0x1;
		    ti1 = s1r[0] & 0x1;
		    tcw = trcw;
		    sk0 = v0r;
		    sk1 = v1r;
		} else {
		    scw = _mm_xor_si128(v0r, v1r);

		    ti0 = s0l[0] & 0x1;
		    ti1 = s1l[0] & 0x1;
		    tcw = tlcw;
		    sk0 = v0l;
		    sk1 = v1l;
		}
		t[2*i]  = tlcw;
		t[2*i+1]= trcw;		

		uint64_t *v = (uint64_t*) &scw;
	    printf(" scw: %.16llx%.16llx  tl:%d tr:%d\n", v[1], v[0], tlcw, trcw);

	    _mm_store_si128((__m128i *)(k0+2*i), scw);
	    _mm_store_si128((__m128i *)(k1+2*i), scw);

	    // compute next seeds
	    if (t0) {  sk0 = _mm_xor_si128(sk0, scw); }
	    if (t1) {  sk1 = _mm_xor_si128(sk1, scw); }
	    _mm_store_si128((__m128i *)s0, sk0);
	    _mm_store_si128((__m128i *)s1, sk1);

	    t0 = ti0 ^ (t0 & tcw);
	    t1 = ti1 ^ (t1 & tcw);
	}

	printf(" *s0: %016llx%016llx|%d\n", s0[1],s0[0], t0);
	printf(" *s1: %016llx%016llx|%d\n", s1[1],s1[0], t1);
	// translate the index point into the index value
	int64_t n0 = s0[0], n1 = s1[0];
	int64_t lcw = value - n0 + n1;
	if (t1) { lcw = -1*lcw; }
	printf(" lcw= %lld\n",lcw);
	k0[2*depth+2] = lcw;
	k1[2*depth+2] = lcw;

}


void fss_eval_step(const uint64_t* k, const uint64_t* cw, uint8_t t, uint8_t tlcw, uint8_t trcw,  uint64_t* l,  uint8_t* tl,  uint64_t* r,  uint8_t *tr) {

	uint64_t left[2] __attribute__ ((aligned (16)));
	uint64_t right[2] __attribute__ ((aligned (16)));
	__m128i vl, vr, vcw;

	fss_expand((unsigned char*)k, (unsigned char*)l, (unsigned char*)r);
	*tl = l[0]&0x1 ^ (t & tlcw);
	*tr = r[0]&0x1 ^ (t & trcw);

	if (t) {
		vl = _mm_load_si128((__m128i *) l);
		vr = _mm_load_si128((__m128i *) r);
		vcw = _mm_load_si128((__m128i *) cw);
		vl = _mm_xor_si128(vl, vcw);
		vr = _mm_xor_si128(vr, vcw);
		_mm_store_si128((__m128i *)l, vl);
		_mm_store_si128((__m128i *)r, vr);
	}

}

// evaluates the dpf at all points in the domain, stores in mem
uint64_t* fss_dpf_eval_all(const uint64_t* key, const uint8_t* t, int size) { 

    uint64_t *small, *big, *a, *b, *u;
    uint8_t* tsmall, *tbig, *ta, *tb, *tu;


    // TODO: expand from the back to the front to remove need for 2nd array
	if (posix_memalign((void**)&small, 16, size*128)!=0 ||
		posix_memalign((void**)&big, 16, size*128)!=0 ||
		posix_memalign((void**)&tsmall, 16, size)!=0 ||
		posix_memalign((void**)&tbig, 16, size)!=0) {
    	fprintf(stderr,"Cannot alloc memory for size %d\n", size);
    	return 0; 
    }

    a=small; b=big;
    ta=tsmall; tb=tbig;

    uint8_t tc=key[0]&0x1, tl, tr;
    fss_eval_step(key, key+2, tc, t[2], t[3], a, ta, (a+2), ta+1); // first step, no advice
    
    printf("[ 0]: %016llx %016llx | %d\n", key[1], key[0], tc);

    for(int j=2, depth=2; j<size; j*=2,depth++) {	// loop over all levels

    	tl = t[2*depth]; tr = t[2*depth+1];

    	for(int k=0; k<j; k++) {	// loop over all nodes in the level
		    tc = ta[k];  // add corrections
		    // printf("[%02d]: %016llx %016llx | %d \n", k, a[2*k+1], a[2*k], tc);

		    fss_eval_step((a+2*k), &key[depth*2], tc, tl, tr, b+4*k, tb+2*k, (b+4*k+2), tb +2*k+1 );

		    // printf("     --> %016llx %016llx|%d    %016llx %016llx|%d\n", b[4*k+1], b[4*k], tb[2*k], b[4*k+3], b[4*k+2], tb[2*k+1]);

    	}

		u=a;a=b;b=u;
		tu=ta;ta=tb;tb=tu;

    }




// int 
// pthread_create (pthread_t *thread_id, const pthread_attr_t *attributes,
//                 void *(*thread_function)(void *), void *arguments);

	free(b); free(tsmall); free(tbig);

	return a;
}

// rom consists of 16-byte lines (128 bits)


void test1() {
	uint64_t k1[2] __attribute__ ((aligned (16))) = { 0x10203ff050607080, 0x90a0b0c0d0e0f000 };
    uint64_t left[2] __attribute__ ((aligned (16))) = { 0, 0 };
    uint64_t right[2] __attribute__ ((aligned (16))) = { 0, 1 };

    uint64_t outl[2] __attribute__ ((aligned (16))) = { 0, 0 };
    uint64_t outr[2] __attribute__ ((aligned (16))) = { 0, 0 };
    uint64_t out2l[2] __attribute__ ((aligned (16))) = { 0, 0 };
    uint64_t out2r[2] __attribute__ ((aligned (16))) = { 0, 0 };
    __m128i v1;
    int i;

    AES_KEY kk;
    // make test vectors

	AES_set_encrypt_key((const unsigned char *)k1, 128, &kk);
    AES_encrypt((const unsigned char *)left, (unsigned char *)outl, &kk);
    AES_encrypt((const unsigned char *)right, (unsigned char *)outr, &kk);
    printf("%llx%llx  %llx%llx\n", outl[1],outl[0], outr[1], outr[0]);

 	//   v1 = _mm_load_si128((__m128i *) _k1);

    fss_expand((unsigned char*)k1, (unsigned char*)out2l, (unsigned char*)out2r);
    printf("%llx%llx  %llx%llx\n", out2l[1],out2l[0], out2r[1], out2r[0]);

}

int main(int argc, char* argv[]) {

	uint64_t _k1[2] __attribute__ ((aligned (16))) = { 0x10203ff050607080, 0x90a0b0c0d0e0f000 };
	uint64_t _k2[2] __attribute__ ((aligned (16))) = { 0x10203ff050607080, 0x90a0b0c0d0e0f000 };

	struct timespec vartime = timer_start();  // begin a timer called 'vartime'


	// leave one more slot for the last advice that maps value from random to beta
	uint64_t _s1[2*12] __attribute__ ((aligned (16)));
	uint64_t _s2[2*12] __attribute__ ((aligned (16)));
	uint8_t  _t[2*11];


	// depth of 10, make f(45)=99
	fss_dpf_compute_key(13, 99, 5, _s1, _s2, _t);


	uint64_t* p1 = fss_dpf_eval_all( _s1, _t, 32);

	uint64_t* p2 = fss_dpf_eval_all( _s2, _t, 32);


    for(int k=0; k<32*2; k+=2) {
   //  	printf("[%04d] %016llx%016llx %016llx%016llx %016llx%016llx\n",k/2,
			// p1[k+1],p1[k],
			// p2[k+1],p2[k],
			// p1[k+1]^p2[k+1],p1[k]^p2[k]);

    	printf("[%04d] %016llu %016llu %016llu\n",k/2,
			p1[k],
			p2[k],
			p1[k]-p2[k]);

    }


	long long time_elapsed = timer_end(vartime);



	printf("Time taken (microsec): %lld\n", time_elapsed);

	return 0;
}