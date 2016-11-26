#pragma once

#include <iostream>

#include <complex> // Must be included before fftw3
#include <fftw3.h>
#include <gd.h>

class QGImage {
public:
	QGImage(int N);
	~QGImage();
	
	void drawLine(const std::complex<double> *fft, const int lineNumber);
	void save(const std::string &fileName);
	
private:
	int N;
	gdImagePtr _im;
	int *_c;
	int _cd; // Color depth
};
