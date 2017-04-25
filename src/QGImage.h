#pragma once

#include <iostream>

#include <complex> // Must be included before fftw3
#include <fftw3.h>
#include <gd.h>

class QGImage {
public:
	QGImage(int size, int sampleRate, int N);
	~QGImage();

	void drawLine(const std::complex<double> *fft, int lineNumber);
	void save2Buffer();
	void save(const std::string &fileName);

	char *getBuffer() { return _imBuffer; };
	int getBufferSize() { return _imBufferSize;} ;

private:
	int _sampleRate;
	int N;
	gdImagePtr _im;
	char *_imBuffer;
	int _imBufferSize;
	int *_c;
	int _cd; // Color depth
};
