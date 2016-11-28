#include <complex>
#include <iostream>

int S = 2000; // Sample rate

inline std::complex<double> genIQ(double f, double t) {
	double wt = 2. * M_PI * f * t;
	return std::complex<double>(cos(wt), sin(wt));
}

int main(int argc, char *argv[]) {
	bool unsignedIQ = false;
	int numSeconds = 1;

	//srand((unsigned) time(NULL));
	
	
	for (double t = 0.; t < numSeconds; t += 1./S) {
		//double r = (double)rand()/RAND_MAX - .5;
		
		//std::complex<double> iq = genIQ(100, t) + genIQ(150, t) + genIQ(-200, t);
		std::complex<double> iq = .1 * genIQ(100, t);

		// Hard clip magnitude to [-1,1] interval
		double iqAbs = abs(iq);
		if (iqAbs > 1) iq /= iqAbs;

		// Shift I/Q to [0,2] interval for unsigned output
		if (unsignedIQ) iq += std::complex<double>(1., 1.);
		
		unsigned char i = ((unsigned char)trunc(127 * iq.real())) & 0xff;
		unsigned char q = ((unsigned char)trunc(127 * iq.imag())) & 0xff;
		
		std::cout << i << q;
	}

	return 0;
}
