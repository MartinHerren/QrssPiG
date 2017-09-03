#include "QGDownSampler.h"

#include <chrono>
#include <complex>
#include <iostream>
#include <liquid/liquid.h>
#include <rtf_common.h>
#include <rtfilter.h>
#include <stdexcept>

QGDownSampler::QGDownSampler(float rate): _rate(rate) {
}

QGDownSampler::~QGDownSampler() {
}

void QGDownSampler::testRTFilter() {
	std::complex<float> *inP = new std::complex<float>[1024+15];
	std::complex<float> *outP = new std::complex<float>[1024+15];
	std::complex<float> *in = (std::complex<float>*)(((uintptr_t)inP + 15) & ~(uintptr_t)0xf);
	std::complex<float> *out = (std::complex<float>*)(((uintptr_t)outP + 15) & ~(uintptr_t)0xf);

	std::cout << "Rate: " << _rate << std::endl;

	hfilter f = rtf_create_downsampler(1, RTF_CFLOAT, _rate);

	for (int chunkSize = 1; chunkSize <= 128; chunkSize *= 2) {
		using namespace std::chrono;
		milliseconds start = duration_cast<milliseconds>(system_clock::now().time_since_epoch());

		int it = 0;
		int ot = 0;

		for (int i = 0; i < (1000*1000)/chunkSize; i++) {
			it += chunkSize;
			ot += rtf_filter(f, in, out, chunkSize);
		}
		milliseconds stop = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
		int t = stop.count() - start.count();

		std::cout << "cs: " << chunkSize << " " << it << " " << ot << " " << (it/ot) << " " << t << "ms " << (1000./t) << "MS/s" << std::endl;
	}


	rtf_destroy_filter(f);

	delete [] inP;
	delete [] outP;
}

void QGDownSampler::testLiquidDsp() {
	std::complex<float> *inP = new std::complex<float>[1024+15];
	std::complex<float> *outP = new std::complex<float>[1024+15];
	std::complex<float> *in = (std::complex<float>*)(((uintptr_t)inP + 15) & ~(uintptr_t)0xf);
	std::complex<float> *out = (std::complex<float>*)(((uintptr_t)outP + 15) & ~(uintptr_t)0xf);

	resamp_crcf q = resamp_crcf_create(1./_rate, 13, .45, 60., 32);

	for (int chunkSize = 1; chunkSize <= 128; chunkSize *= 2) {
		using namespace std::chrono;
		milliseconds start = duration_cast<milliseconds>(system_clock::now().time_since_epoch());

		int it = 0;
		int ot = 0;
		
		for (int i = 0; i < (1000*1000)/chunkSize; i++) {
			it += chunkSize;
			unsigned int o;
			resamp_crcf_execute_block(q, in, chunkSize, out, &o);
			ot += o;
		}
		milliseconds stop = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
		int t = stop.count() - start.count();

		std::cout << "cs: " << chunkSize << " " << it << " " << ot << " " << (it/ot) << " " << t << "ms " << (1000./t) << "MS/s" << std::endl;
	}

	resamp_crcf_destroy(q);
}
