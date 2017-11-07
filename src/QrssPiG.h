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
private:
	QrssPiG();

public:
	QrssPiG(const std::string &format, int sampleRate, int N, const std::string &dir, const std::string &sshHost, const std::string &sshUser, int sshPort);
	QrssPiG(const std::string &configFile);
	~QrssPiG();

	void run();
	void stop();

private:
	void _init();

	void _addIQ(const std::complex<float> *iq, unsigned int len);
	void _computeFft();
	void _pushIntermediateImage();
	void _pushImage(bool wait = false);

	// Input device
	std::unique_ptr<QGInputDevice> _inputDevice;

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
	std::unique_ptr<float[]> _hannW;
	std::complex<float> *_fftIn;
	std::complex<float> *_fftOut;

	std::unique_ptr<QGDownSampler> _resampler;
	QGFft *_fft;

	// Image
	std::unique_ptr<QGImage> _im;

	// Uploaders
	std::vector<std::unique_ptr<QGUploader>> _uploaders;

	int _frameIndex;
};
