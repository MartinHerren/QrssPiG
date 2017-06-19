#pragma once

#include <chrono>
#include <complex>
#include <iostream>
#include <string>

#include "QGFft.h"
#include "QGImage.h"
#include "QGUploader.h"

class QrssPiG {
private:
	QrssPiG();

public:
	QrssPiG(int N, bool unsignedIQ, int sampleRate);
	QrssPiG(const std::string &configFile);
	~QrssPiG();

	void addUploader(const std::string &sshHost, const std::string &sshUser, const std::string &sshDir, int sshPort);

	void addIQ(std::complex<double> iq);

private:
	void _init();
	void _timeInit();

	void _computeFft();
	void _applyFilter();
	void _pushImage();

private:
	// FFT size
	int _N;

	// Data format
	bool _unsignedIQ;
	int _sampleRate;

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
