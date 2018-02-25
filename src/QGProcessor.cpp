#include "QGProcessor.h"

#include <cstring>
#include <iostream>
#include <math.h>
#include <stdexcept>

#ifdef HAVE_LIBLIQUIDSDR
#else
#ifdef HAVE_LIBRTFILTER
#include <rtf_common.h>
#endif // HAVE_LIBRTFILTER
#endif // HAVE_LIBLIQUIDSDR

QGProcessor::QGProcessor(const YAML::Node &config) {
	unsigned int iSampleRate = 48000;

	_sampleRate = 0;
	_chunkSize = 32;
	_N = 2048;
	_overlap = (3 * _N) / 4; // As it depends on N, default initialization will be redone later even when not given in config
	_rate = 1.;

	// input samplerate should already be set to default value by input device if it didn't exist
	if (config["input"]) {
		YAML::Node input = config["input"];

		if (input["samplerate"]) iSampleRate = input["samplerate"].as<int>();
	}

	if (config["processing"]) {
		YAML::Node processing = config["processing"];

		if (processing["samplerate"]) _sampleRate = processing["samplerate"].as<int>();
		if (processing["chunksize"]) _chunkSize = processing["chunksize"].as<int>();
		if (processing["fft"]) _N = processing["fft"].as<int>();
		if (processing["fftoverlap"]) {
			int o = processing["fftoverlap"].as<int>();
			if ((o < 0) || (o >= _N)) throw std::runtime_error("YAML: overlap value out of range [0..N[");
			_overlap = (o * _N) / (o + 1);
		} else {
			_overlap = (3 * _N) / 4;
		}
	}

	if (_sampleRate && (iSampleRate != _sampleRate)) {
		_rate = (float)iSampleRate / (float)_sampleRate;

		// Only libliquid supports integer rates. Taking floor to get the next higher end samplerate otherwise
#ifndef HAVE_LIBLIQUIDSDR
		_rate = floor(_rate);
#endif // HAVE_LIBLIQUIDSDR

		// Patch config with real samplerate from resampler
		_sampleRate = iSampleRate / _rate;
	}

	if (_sampleRate == 0) {
		_sampleRate = iSampleRate;
	}

	_inputIndex = 0;
	// Add provision to add a full chunksize when only one sample left before reaching N. If resampling, will be smaller in worst case
	_input.reset(new std::complex<float>[_N + _chunkSize - 1]);

	if (_rate != 1.0) {
#ifdef HAVE_LIBLIQUIDSDR
		_liquidSdrResampler = resamp_crcf_create(1./_rate, 6000, .49/_rate, 60., 32);
#else
#ifdef HAVE_LIBRTFILTER
		_rtFilterResampler = rtf_create_downsampler(1, RTF_CFLOAT, (unsigned int)_rate);
#else
		_counter = 0;
#endif // HAVE_LIBRTFILTER
#endif // HAVE_LIBLIQUIDSDR
	}

	_hannW.reset(new float[_N]);

	for (int i = 0; i < _N/2; i++) {
		_hannW[i] = .5 * (1 - cos((2 * M_PI * i) / (_N / 2 - 1)));
	}

	// TODO: enable to chose patient or exaustive plan creation. Save and restore plan
	_fftIn = (std::complex<float>*)fftwf_malloc(sizeof(std::complex<float>) * _N);
	_fftOut = (std::complex<float>*)fftwf_malloc(sizeof(std::complex<float>) * _N);
	_plan = fftwf_plan_dft_1d(_N, reinterpret_cast<fftwf_complex*>(_fftIn), reinterpret_cast<fftwf_complex*>(_fftOut), FFTW_FORWARD, FFTW_MEASURE);
}

QGProcessor::~QGProcessor() {
	fftwf_destroy_plan(_plan);
	fftwf_free(_fftIn);
	fftwf_free(_fftOut);

	if (_rate != 1.0) {
#ifdef HAVE_LIBLIQUIDSDR
		resamp_crcf_destroy(_liquidSdrResampler);
#else
#ifdef HAVE_LIBRTFILTER
		rtf_destroy_filter(_rtFilterResampler);
#endif // HAVE_LIBRTFILTER
#endif // HAVE_LIBLIQUIDSDR
	}
}

void QGProcessor::addCb(std::function<void(const std::complex<float>*)>cb) {
	_cbs.push_back(cb);
}

void QGProcessor::addIQ(const std::complex<float> *iq) {
	if (_rate != 1.0) {
		_inputIndex += _resample(iq, _input.get() + _inputIndex);
	} else {
		std::memcpy(_input.get() + _inputIndex, iq, _chunkSize * sizeof(std::complex<float>));
		_inputIndex += _chunkSize;
	}

	if (_inputIndex >= _N) {
		_fft();

		// Shift overlap to buffer start
		for (auto i = 0; i < _overlap + _inputIndex - _N; i++) _input[i] = _input[_N - _overlap + i];
		_inputIndex = _overlap + _inputIndex - _N;
	}
}

unsigned int QGProcessor::_resample(const std::complex<float> *in, std::complex<float> *out) {
	unsigned int outSize = 0;

#ifdef HAVE_LIBLIQUIDSDR
	resamp_crcf_execute_block(_liquidSdrResampler, const_cast<std::complex<float>*>(in), _chunkSize, out, &outSize);
#else
#ifdef HAVE_LIBRTFILTER
	outSize = rtf_filter(_rtFilterResampler, in, out, _chunkSize);
#else
	for (unsigned int i = 0; i < _chunkSize; i++) { // Reuse i, so outer while loop will run only once
		if (!_counter++) out[outSize++] = in[i];
		if (_counter >= _rate) _counter = 0;
	}
#endif // HAVE_LIBRTFILTER
#endif // HAVE_LIBLIQUIDSDR

	return outSize;
}

void QGProcessor::_fft() {
	for (int i = 0; i < _N; i++) _fftIn[i] = _input.get()[i] * _hannW[i / 2];
	fftwf_execute(_plan);

	for (auto& cb: _cbs) cb(_fftOut);
}
