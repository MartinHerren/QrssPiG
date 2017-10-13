#pragma once

#include <complex>
#include <iostream>
#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "QGDownSampler.h"
#include "QGFft.h"
#include "QGImage.h"
#include "QGInputDevice.h"
#include "QGUploader.h"

class QrssPiG {
public:
	enum class Format { U8IQ, S8IQ, U16IQ, S16IQ, S16MONO, S16LEFT, S16RIGHT };

private:
	QrssPiG();

public:
	QrssPiG(const std::string &format, int sampleRate, int N, const std::string &dir, const std::string &sshHost, const std::string &sshUser, int sshPort);
	QrssPiG(const std::string &configFile);
	~QrssPiG();

	void run();

	void stop() { _running = false; };

private:
	void _addUploader(const YAML::Node &uploader);
	void _init();

	void _addIQ(std::complex<float> iq);
	void _computeFft();
	void _pushImage(bool wait = false);

	// Input device
	QGInputDevice *_inputDevice;

	// Input data format
	Format _format;
	int _sampleRate;
	int _baseFreq;

	// Processing
	int _chunkSize;
	int _N;
	int _overlap; // 0: no overlap, 1: 1/2, 2: 2/3...

	// Processing buffer
	int _inputIndex;
	int _inputIndexThreshold;
	int _resampledIndex;
	std::complex<float> *_input;
	std::complex<float> *_resampled;
	float *_hannW;
	std::complex<float> *_fftIn;
	std::complex<float> *_fftOut;

	QGDownSampler *_resampler;
	QGFft *_fft;
	QGImage *_im;
	std::vector<QGUploader*> _uploaders;

	int _frameIndex;

	volatile bool _running;
};
