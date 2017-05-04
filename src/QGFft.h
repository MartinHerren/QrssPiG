#pragma once

#include <complex> // Must be included before fftw3
#include <fftw3.h>

class QGFft {
public:
	QGFft(int N);
	~QGFft();

	std::complex<double> *getInputBuffer() { return _in; };
	std::complex<double> *getFftBuffer() { return _fft; };

	void process();
	void average();
	void reset();

private:
	int N;

	fftw_plan _p;
	std::complex<double> *_in;
	std::complex<double> *_out;
	std::complex<double> *_fft;
	int _count;
};
