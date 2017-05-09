#include "QGFft.h"

#include <stdexcept>

QGFft::QGFft(int N): N(N) {
	_in = (std::complex<double>*)fftw_malloc(sizeof(std::complex<double>) * N);
	_out = (std::complex<double>*)fftw_malloc(sizeof(std::complex<double>) * N);
	_fft = (std::complex<double>*)fftw_malloc(sizeof(std::complex<double>) * N);

	_p = fftw_plan_dft_1d(N, reinterpret_cast<fftw_complex*>(_in), reinterpret_cast<fftw_complex*>(_out), FFTW_FORWARD, FFTW_ESTIMATE);

	// as _fft is accumulated, it must be initialized to zero first, resets _count as well
	reset();
}

QGFft::~QGFft() {
	fftw_destroy_plan(_p);

	fftw_free(_in);
	fftw_free(_out);
	fftw_free(_fft);
}

void QGFft::process() {
	fftw_execute(_p);

	for (auto i = 0; i < N; i++) _fft[i] += _out[i];

	_count++;
}

void QGFft::average() {
	if (_count == 0) throw std::runtime_error("No data to average");

	for (auto i = 0; i < N; i++) _fft[i] /= _count;

	_count = 1; // TODO: Probably better to set a flag as it doesn't make sense to process after an average without reset
}

void QGFft::reset() {
	for (auto i = 0; i < N; i++) _fft[i] = std::complex<double>(0.,0.);

	_count = 0;
}
