#pragma once

#include <complex>

class QGDownSampler {
public:
	QGDownSampler(float rate, unsigned int cs);
	~QGDownSampler();

	void testRTFilter();
	void testLiquidDsp();

private:
	float _rate;
	unsigned int _cs;

	// internal
	std::complex<float> *_inAllocated;
	std::complex<float> *_outAllocated;
	std::complex<float> *_in;
	std::complex<float> *_out;
};
