#pragma once

#include <complex>
#include <iostream>
#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "QGFft.h"
#include "QGImage.h"
#include "QGUploader.h"

class QrssPiG {
public:
	enum class Format { U8IQ, S8IQ, U16IQ, S16IQ };

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

	void _addIQ(std::complex<double> iq);
	void _computeFft();
	void _pushIntermediateImage();
	void _pushImage(bool wait = false);

	// FFT size
	int _N;
	int _overlap; // 0: no overlap, 1: 1/2, 2: 2/3...

	// Data format
	Format _format;
	int _sampleRate;
	int _baseFreq;

	double *_hannW;
	QGFft *_fft;

	int _idx; // Current sample index in fft input

	std::complex<double> *_in;
	std::complex<double> *_fftIn;
	std::complex<double> *_fftOut;

	QGImage *_im;
	std::vector<QGUploader*> _uploaders;

	int _frameIndex;

	volatile bool _running;
};
