
#include <stdio.h>
#include <stdlib.h>


// find the first coefficient c_3 of (n-choose-3) representation
// finds the (g choose 3) value that is closest to, but less than ind
int approx3(int ind) {
	int g = 1000;

	for(int j=0; j<16; j++) {
		long g2 = g*g;
		long g3 = g2*g;
		long f  = g3 - 3*g2 + 2*g - 6*ind;
		long fp = 3*g2 - 6*g + 2;

		int ng = g - (f/fp);
		if (f>0 && fp>f) { ng--; }

		// if (ng==g) {
			 //printf("     %2d    %05ld  %05ld %3d --> %3d\n",j,f,fp,g, ng);
		// }

		g = ng;
	}

	if ( g*(g-1)*(g-2) > 6*ind ) { g--; }

	return g;
}



// approximates the (g choose 2) value that is closest to ind
int approx2(int ind) {

	// f(x) = g*(g-1) - 2*ind = g^2 - g - 2*ind
	// looking for integeral x that produces smallest positive value of f
	// f'(x) = 2*g - 1

	int  x = 999;
	for(int j=0; j<16; j++) {
		long x2 = x*x;
		long f  = x2 - x - 2*ind;
		long fp = 2*x - 1;

		int nx = x - (f/fp);
		if (f>0 && fp>f) { nx--; }

		x = nx;
	}

	if ( x*(x-1) > 2*ind ) { x--; }

	return x;

}


// returns index for a given sequence
int map(int c3, int c2, int c1) {
	// return (c3-choose-3) + (c2-choose-2) + c1
	int t3 = c3*(c3-1)*(c3-2)/6;
	int t2 = c2*(c2-1)/2;
	return t3 + t2 + c1;

}


void query(int ind, int* c3, int* c2, int* c1) {

	int h3  = approx3(ind);
	int hl3 = h3*(h3-1)*(h3-2)/6;

	int i2 = ind-hl3;
	int h2 = approx2(i2);

	int h2l = h2*(h2-1)/2;

	int h1 = i2 - h2l;

	*c3 = h3; *c2 = h2; *c1 = h1;
	return;

	// printf("%6d   %3d %3d %3d   [hl:%5d h2l:%4d]\n",i,h,h2,h3,hl,h2l);


}


int main() {


	// for(int i2=1; i2<10000; i2++) {
	// 	int h2 = approx2(i2);

	// 	int h2l = h2*(h2-1)/2;

	// 	printf("%5d   %3d  %5d \n",i2, h2, h2l);

	// }



	int oh = -1;
	for(int i=1; i<10000000; i++) {
		int h = approx3(i);
		int hl = h*(h-1)*(h-2)/6;
		int hh = (h+1)*h*(h-1)/6;

		int i2 = i-hl;
		int h2 = approx2(i2);

		int h2l = h2*(h2-1)/2;

		int h3 = i2 - h2l;

		if (i != map(h,h2,h3)) {
			printf("!!! error at %d (%d %d %d)\n",i,h,h2,h3);
			break;
		}


	}

	return 0;
}