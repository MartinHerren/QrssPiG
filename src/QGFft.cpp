#include "QGFft.h"

QGFft::QGFft(int N): N(N) {
	_in = (std::complex<double>*)fftw_malloc(sizeof(std::complex<double>) * N);
	_out = (std::complex<double>*)fftw_malloc(sizeof(std::complex<double>) * N);
	
	_p = fftw_plan_dft_1d(N, reinterpret_cast<fftw_complex*>(_in), reinterpret_cast<fftw_complex*>(_out), FFTW_FORWARD, FFTW_ESTIMATE);
}

QGFft::~QGFft() {
	fftw_destroy_plan(_p);
	
	fftw_free(_in);
	fftw_free(_out);
}

void QGFft::process() {
	fftw_execute(_p);
}
