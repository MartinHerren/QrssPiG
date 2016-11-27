#include <iostream>
#include <string>

#include <complex>

#include <math.h>
#include <stdlib.h>

#include "QGFft.h"
#include "QGImage.h"


int N = 4096; // FFT size
int S = 1000; // Sample rate

// S / N => Herz per bucket: .5

QGFft *fft;
QGImage *im;

double hannW[8000];

int main(int argc, char *argv[]) {
	std::string s("test");
	srand((unsigned) time(NULL));
	
	fft = new QGFft(N);
	im = new QGImage(S, N);
	
	std::complex<double> *in = fft->getInputBuffer();
	std::complex<double> *out = fft->getOutputBuffer();
	
	for (int i = 0; i < N/2; i++) {
		hannW[i] = .5 * (1 - cos((2 * M_PI * i) / (N / 2 - 1)));
//		hannW[i] = 1;
	}
	
	//std::cin >> std::noskipws;
	int idx = 0;
	int y = 0;
	int p = 0;
	int resampleCounter = 0;
	int resampleValue = 1;
	unsigned char i, q;
	
	while (std::cin >> i >> q) {
		//using namespace std::complex_literals;
		if (resampleCounter == 0) {
		in[idx].real((2. * i / 255 - 1) * hannW[idx/2]);
		in[idx].imag((2 * q / 255 - 1) * hannW[idx/2]);
		idx++;
		
		if (idx >= N) {
			fft->process();
			im->drawLine(out, y++);
			idx = 0;
		}
			
		//std::cout << (unsigned int)i << " " << (unsigned int)q << " " << creal(in[idx]) << std::endl;
		
		if (y > 2000) {
			im->save(s + std::to_string(p++) + ".png");
			y = 0;
		}
		}
		
		resampleCounter++;
		if (resampleCounter == resampleValue) resampleCounter = 0;
	}
	
	im->save(s + std::to_string(p++) + ".png");
	
	delete im;
	delete fft;

	return 0;
}
