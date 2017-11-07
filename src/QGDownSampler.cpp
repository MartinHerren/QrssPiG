#include "QGDownSampler.h"

#include <iostream>
#include <math.h>
#include <stdexcept>

#ifdef HAVE_LIBLIQUIDSDR
#else
#ifdef HAVE_LIBRTFILTER
#include <rtf_common.h>
#endif // HAVE_LIBRTFILTER
#endif // HAVE_LIBLIQUIDSDR

QGDownSampler::QGDownSampler(float rate, unsigned int cs): _cs(cs) {
#ifdef HAVE_LIBLIQUIDSDR
	_rate = rate;
	_liquidSdrResampler = resamp_crcf_create(1./_rate, 6000, .49/_rate, 60., 32);
#else
#ifdef HAVE_LIBRTFILTER
	_rate = floor(rate); // Only integer rates supported. Taking floor to get the next higher end samplerate
	_rtFilterResampler = rtf_create_downsampler(1, RTF_CFLOAT, (unsigned int)_rate);
#else
	_rate = floor(rate);
	_counter = 0;
	_counterLimit = (unsigned int)floor(_rate);
#endif // HAVE_LIBRTFILTER
#endif // HAVE_LIBLIQUIDSDR
}

QGDownSampler::~QGDownSampler() {
#ifdef HAVE_LIBLIQUIDSDR
	resamp_crcf_destroy(_liquidSdrResampler);
#else
#ifdef HAVE_LIBRTFILTER
	rtf_destroy_filter(_rtFilterResampler);
#endif // HAVE_LIBRTFILTER
#endif // HAVE_LIBLIQUIDSDR
}

float QGDownSampler::getRealRate() {
	return _rate;
}

unsigned int QGDownSampler::processChunk(const std::complex<float> *in, std::complex<float> *out) {
	unsigned int outSize = 0;

#ifdef HAVE_LIBLIQUIDSDR
	resamp_crcf_execute_block(_liquidSdrResampler, const_cast<std::complex<float>*>(in), _cs, out, &outSize);
#else
#ifdef HAVE_LIBRTFILTER
	outSize = rtf_filter(_rtFilterResampler, in, out, _cs);
#else
	for (int i = 0; i < _cs; i++) { // Reuse i, so outer while loop will run only once
		if (!_counter++) out[outSize++] = in[i];
		if (_counter >= _counterLimit) _counter = 0;
	}
#endif // HAVE_LIBRTFILTER
#endif // HAVE_LIBLIQUIDSDR

	return outSize;
}

unsigned int QGDownSampler::process(const std::complex<float> *in, unsigned int inSize, std::complex<float> *out) {
	unsigned int remSize = inSize;
	unsigned int outSize = 0;

	while (remSize) {
		unsigned int i = (remSize > _cs) ? _cs : remSize;
		unsigned int o = 0;

#ifdef HAVE_LIBLIQUIDSDR
		resamp_crcf_execute_block(_liquidSdrResampler, const_cast<std::complex<float>*>(in), i, out, &o);
#else
#ifdef HAVE_LIBRTFILTER
		o = rtf_filter(_rtFilterResampler, in, out, i);
#else
		for (i = 0; i < remSize; i++) { // Reuse i, so outer while loop will run only once
			if (!_counter++) out[o++] = in[i];
			if (_counter >= _counterLimit) _counter = 0;
		}
#endif // HAVE_LIBRTFILTER
#endif // HAVE_LIBLIQUIDSDR

		remSize -= i;
		outSize += o;
		in += i;
		out += o;
	}

	return outSize;
}
