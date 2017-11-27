#pragma once

#include "Config.h"

#include <complex> // Must be included before fftw3
#include <functional>
#include <memory>
#include <vector>

#include <fftw3.h>
#include <yaml-cpp/yaml.h>

#ifdef HAVE_LIBLIQUIDSDR
#include <liquid/liquid.h>
#else
#ifdef HAVE_LIBRTFILTER
#include <rtfilter.h>
#endif // HAVE_LIBRTFILTER
#endif // HAVE_LIBLIQUIDSDR

class QGProcessor {
public:
	QGProcessor(const YAML::Node &config);
	~QGProcessor();

	void addCb(std::function<void(const std::complex<float>*)>cb);

	void addIQ(const std::complex<float> *iq);

	unsigned int sampleRate() { return _sampleRate; };
	unsigned int chunkSize() { return _chunkSize; };
	int fftSize() { return _N; };
	int fftOverlap() { return _overlap; };
	float downsamplingRate() { return _rate; };

private:
	unsigned int _resample(const std::complex<float> *in, std::complex<float> *out);
	void _fft();

	unsigned int _sampleRate;
	unsigned int _chunkSize;
	int _N;
	int _overlap; // 0: no overlap, 1: 1/2, 2: 2/3...
	float _rate;

	// Input buffer
	int _inputIndex;
	std::unique_ptr<std::complex<float>[]> _input;

	// Resampler
#ifdef HAVE_LIBLIQUIDSDR
	resamp_crcf _liquidSdrResampler;
#else
#ifdef HAVE_LIBRTFILTER
	hfilter _rtFilterResampler;
#else
	unsigned int _counter;
#endif // HAVE_LIBRTFILTER
#endif // HAVE_LIBLIQUIDSDR

	// Window function
	std::unique_ptr<float[]> _hannW;

	// FFT
	fftwf_plan _plan;
	std::complex<float> *_fftIn;
	std::complex<float> *_fftOut;

	std::vector<std::function<void(const std::complex<float>*)>> _cbs;
};
