#include "QGFft.h"

#include <stdexcept>

QGFft::QGFft(int N) {
	_in = (std::complex<float>*)fftwf_malloc(sizeof(std::complex<float>) * N);
	_out = (std::complex<float>*)fftwf_malloc(sizeof(std::complex<float>) * N);

	_p = fftwf_plan_dft_1d(N, reinterpret_cast<fftwf_complex*>(_in), reinterpret_cast<fftwf_complex*>(_out), FFTW_FORWARD, FFTW_ESTIMATE);
}

QGFft::~QGFft() {
	fftwf_destroy_plan(_p);

	fftwf_free(_in);
	fftwf_free(_out);
}

void QGFft::process() {
	fftwf_execute(_p);
}
