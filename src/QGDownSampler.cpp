#include "QGDownSampler.h"

#include <chrono>
#include <iostream>
#include <liquid/liquid.h>
#include <math.h>
#include <rtf_common.h>
#include <rtfilter.h>
#include <stdexcept>

QGDownSampler::QGDownSampler(float rate, unsigned int cs): _rate(rate), _cs(cs) {
	_inAllocated = new std::complex<float>[_cs+15]; // +15 to be able to enforce 16byte alignement
	_outAllocated = new std::complex<float>[(int)ceil(1.1/_rate*_cs)+15]; // 10% safe for flexible filter output
	_in = (std::complex<float>*)(((uintptr_t)_inAllocated + 15) & ~(uintptr_t)0xf);
	_out = (std::complex<float>*)(((uintptr_t)_outAllocated + 15) & ~(uintptr_t)0xf);
}

QGDownSampler::~QGDownSampler() {
	delete [] _inAllocated;
	delete [] _outAllocated;
}

void QGDownSampler::testRTFilter() {
	hfilter f = rtf_create_downsampler(1, RTF_CFLOAT, _rate);

	for (unsigned int chunkSize = 1; chunkSize <= _cs; chunkSize *= 2) {
		using namespace std::chrono;
		milliseconds start = duration_cast<milliseconds>(system_clock::now().time_since_epoch());

		int it = 0;
		int ot = 0;
		int mo = 0;

		for (unsigned int i = 0; i < (8*1000*1000)/chunkSize; i++) {
			it += chunkSize;
			int o = rtf_filter(f, _in, _out, chunkSize);
			ot += o;
			if (o > mo) mo = o;
		}
		milliseconds stop = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
		int t = stop.count() - start.count();

		std::cout << "cs: " << chunkSize << " " << it << " " << ot << " " << ((float)it/ot) << " " << t << "ms " << (8*1000./t) << "MS/s " << mo << std::endl;
	}

	rtf_destroy_filter(f);
}

void QGDownSampler::testLiquidDsp() {
	resamp_crcf q = resamp_crcf_create(1./_rate, 13, .45, 60., 32);

	for (unsigned int chunkSize = 1; chunkSize <= _cs; chunkSize *= 2) {
		using namespace std::chrono;
		milliseconds start = duration_cast<milliseconds>(system_clock::now().time_since_epoch());

		int it = 0;
		int ot = 0;
		unsigned int mo = 0;
		
		for (unsigned int i = 0; i < (8*1000*1000)/chunkSize; i++) {
			it += chunkSize;
			unsigned int o;
			resamp_crcf_execute_block(q, _in, chunkSize, _out, &o);
			ot += o;
			if (o > mo) mo = o;
		}
		milliseconds stop = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
		int t = stop.count() - start.count();

		std::cout << "cs: " << chunkSize << " " << it << " " << ot << " " << ((float)it/ot) << " " << t << "ms " << (8*1000./t) << "MS/s " << mo << std::endl;
	}

	resamp_crcf_destroy(q);
}
