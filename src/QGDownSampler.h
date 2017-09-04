#pragma once

#include "Config.h"

#include <complex>

class QGDownSampler {
public:
	QGDownSampler(float rate, unsigned int cs);
	~QGDownSampler();

#ifdef HAVE_LIBRTFILTER
	void testRTFilter();
#endif // HAVE_LIBRTFILTER
#ifdef HAVE_LIBLIQUIDSDR
	void testLiquidDsp();
#endif // HAVE_LIBLIQUIDSDR

private:
	float _rate;
	unsigned int _cs;

	// internal
	std::complex<float> *_inAllocated;
	std::complex<float> *_outAllocated;
	std::complex<float> *_in;
	std::complex<float> *_out;
};
