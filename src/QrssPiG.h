#pragma once

#include <chrono>
#include <complex>
#include <iostream>
#include <string>

#include <yaml-cpp/yaml.h>

#include "QGFft.h"
#include "QGImage.h"
#include "QGUploader.h"

class QrssPiG {
private:
	QrssPiG();

public:
	QrssPiG(int N, bool unsignedIQ, int sampleRate, const std::string &dir, const std::string &sshHost, const std::string &sshUser, int sshPort);
	QrssPiG(const std::string &configFile);
	~QrssPiG();

	void run();

private:
	void _addUploader(const YAML::Node &uploader);
	void _init();
	void _timeInit();

	void _addIQ(std::complex<double> iq);
	void _computeFft();
	void _pushImage();

private:
	// FFT size
	int _N;
	int _overlap; // 0: no overlap, 1: 1/2, 2: 2/3...

	// Data format
	bool _unsignedIQ;
	int _sampleRate;
	int _baseFreq;

	// Image format
	int _secondsPerFrame;
	int _frameSize;

	double _samplesPerLine;
	double _linesPerSecond;

	double *_hannW;
	QGFft *_fft;

	int _idx;
	long _samples;

	std::complex<double> *_in;
	std::complex<double> *_fftIn;
	std::complex<double> *_fftOut;

	QGImage *_im;
	QGUploader *_up;

	int _lastLine;
	int _lastFrame;

	std::chrono::milliseconds _started;
};
