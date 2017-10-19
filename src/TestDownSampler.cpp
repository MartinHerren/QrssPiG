#include "Config.h"

#include <chrono>
#include <iostream>

#include "QGDownSampler.h"

using namespace std::chrono;

int main(int argc, char *argv[]) {
	(void)argc;(void)argv;
	float rate = 8000./6;

	unsigned int inSize = 1*2666000;
	unsigned int remSize = inSize;
	unsigned int outSize = 0;

	unsigned int inBlockSize = 1000;
	unsigned int outBlockSize = (int)ceil(1.1 * inBlockSize / rate) + 4;

	std::complex<float> *inA = new std::complex<float>[inBlockSize + 15];
	std::complex<float> *outA = new std::complex<float>[outBlockSize + 15];
	std::complex<float> *in = (std::complex<float>*)(((uintptr_t)inA + 15) & ~(uintptr_t)0xf);
	std::complex<float> *out = (std::complex<float>*)(((uintptr_t)outA + 15) & ~(uintptr_t)0xf);

	std::cout << inA << " " << outA << "->" << in << " " << out << std::endl;

	QGDownSampler *d = new QGDownSampler(rate, 32);
	std::cout << rate << "->" << d->getRealRate() << " " << inSize << "->" << (inSize/d->getRealRate()) << std::endl;

	milliseconds start = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
	while (remSize) {
		unsigned int i = (remSize > inBlockSize) ? inBlockSize : remSize;
		outSize += d->process(in, i, out);

		remSize -= i;
	}
	milliseconds stop = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
	int t = stop.count() - start.count();
	std::cout << inSize << "->" << outSize << " Bytes in " << t << "ms: " << inSize/(1000.*t) << "MS/s " << std::endl;

	delete d;
	delete [] inA;
	delete [] outA;

	return 0;
}
