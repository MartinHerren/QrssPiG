#pragma once

#include "Config.h"

#include <complex>

#ifdef HAVE_LIBLIQUIDSDR
#include <liquid/liquid.h>
#else
#ifdef HAVE_LIBRTFILTER
#include <rtfilter.h>
#endif // HAVE_LIBRTFILTER
#endif // HAVE_LIBLIQUIDSDR

class QGDownSampler {
public:
	QGDownSampler(float rate, unsigned int cs);
	~QGDownSampler();

	float getRealRate();
	unsigned int processChunk(const std::complex<float> *in, std::complex<float> *out);
	unsigned int process(const std::complex<float> *in, unsigned int inSize, std::complex<float> *out);

private:
	float _rate;
	unsigned int _cs;

	// internal
#ifdef HAVE_LIBLIQUIDSDR
	resamp_crcf _liquidSdrResampler;
#else
#ifdef HAVE_LIBRTFILTER
	hfilter _rtFilterResampler;
#else
	unsigned int _counter;
	unsigned int _counterLimit;
#endif // HAVE_LIBRTFILTER
#endif // HAVE_LIBLIQUIDSDR
};
