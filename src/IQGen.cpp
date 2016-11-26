#include <iostream>

#include <math.h>
#include <stdlib.h>

int S = 2000; // Sample rate

inline double genI(double t) {
	double v;
	
	v = sin(2. * M_PI * 10 * t) + .7 * sin(2. * M_PI * 30 * t) + .5 * sin(2. * M_PI * 70 * t);
	
	// Hard clip result
	if (v > 1.0) v = 1.0;
	if (v < -1.0) v = -1.0;
	
	return v;
}

inline double genQ(double t) {
	return 0. * t;
}

int main(int argc, char *argv[]) {
	srand((unsigned) time(NULL));
	
	int numSeconds = 120;
	
	for (double t = 0.; t < numSeconds; t += 1./S) {
		//double r = (double)rand()/RAND_MAX - .5;
		
		unsigned char i =  ((unsigned char)trunc(255./2 * (1 + genI(t)))) & 0xff;
		unsigned char q =  ((unsigned char)trunc(255./2 * (1 + genQ(t)))) & 0xff;
		
		std::cout << i << q;
	}

	return 0;
}
