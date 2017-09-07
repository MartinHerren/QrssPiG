#pragma once

#include <complex> // Must be included before fftw3
#include <fftw3.h>

class QGFft {
public:
	QGFft(int N);
	~QGFft();

	std::complex<float> *getInputBuffer() { return _in; };
	std::complex<float> *getFftBuffer() { return _out; };

	void process();

private:
	fftwf_plan _p;
	std::complex<float> *_in;
	std::complex<float> *_out;
};
