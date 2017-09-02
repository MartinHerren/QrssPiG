#include "QGDownSampler.h"

#include <chrono>
#include <complex>
#include <iostream>
#include <rtf_common.h>
#include <rtfilter.h>
#include <stdexcept>

QGDownSampler::QGDownSampler(unsigned int rate): _rate(rate) {
}

QGDownSampler::~QGDownSampler() {
}

void QGDownSampler::testRTFilter() {
	std::complex<double> *inP = new std::complex<double>[2*1024+15];
	std::complex<double> *outP = new std::complex<double>[2*1024+15];
	std::complex<double> *in = (std::complex<double>*)(((uintptr_t)inP + 15) & ~(uintptr_t)0xf);
	std::complex<double> *out = (std::complex<double>*)(((uintptr_t)outP + 15) & ~(uintptr_t)0xf);

	std::cout << "Rate: " << _rate << std::endl;

	hfilter f = rtf_create_downsampler(2, RTF_CDOUBLE, _rate);

	for (int chunkSize = 1; chunkSize <= 128; chunkSize *= 2) {
		using namespace std::chrono;
		milliseconds start = duration_cast<milliseconds>(system_clock::now().time_since_epoch());

		int it = 0;
		int ot = 0;

		for (int i = 0; i < (1000*1000)/chunkSize; i++) {
			it += chunkSize;
			ot += rtf_filter(f, in + 1, out, chunkSize);
		}
		milliseconds stop = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
		int t = stop.count() - start.count();

		std::cout << "cs: " << chunkSize << " " << it << " " << ot << " " << (it/ot) << " " << t << "ms " << (1000./t) << "MS/s" << std::endl;
	}


	rtf_destroy_filter(f);

	delete [] inP;
	delete [] outP;
}
