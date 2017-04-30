#include "QrssPiG.h"

#include <chrono>
#include <string>

QrssPiG::QrssPiG(bool unsignedIQ, int sampleRate, int N) :
	_unsignedIQ(unsignedIQ),
	_sampleRate(sampleRate),
	_N(N),
	_secondsPerFrame(600),
	_frameSize(1000) {
	_init();
}

QrssPiG::~QrssPiG() {
	_pushImage();

	if (_hannW) delete [] _hannW;
	if (_fft) delete _fft;
	if (_im) delete _im;
	if (_up) delete _up;
}

void QrssPiG::addUploader(const std::string &sshHost, const std::string &sshUser, const std::string &sshDir, int sshPort) {
	if (_up) delete _up;
	_up = new QGUploader(sshHost, sshUser, sshDir, sshPort);
}

void QrssPiG::addIQ(std::complex<double> iq) {
	// Shift I/Q from [0,2] to [-1,1} interval for unsigned input
	if (_unsignedIQ) iq -= std::complex<double>(-1.,-1);

	_fftIn[_idx++] = iq;

	if (_idx >= _N) {
		_computeFft();
		_idx = 0;
	}
}

void QrssPiG::_init() {
	_fft = new QGFft(_N);

	_fftIn = _fft->getInputBuffer();
	_fftOut = _fft->getFftBuffer();

	_linesPerSecond = (double)_frameSize / _secondsPerFrame;
	_lastLine = -1;
	_lastFrame = -1;

	_im = new QGImage(_frameSize, _sampleRate, _N);
	_up = nullptr;

	_hannW = new double[_N];

	for (int i = 0; i < _N/2; i++) {
		_hannW[i] = .5 * (1 - cos((2 * M_PI * i) / (_N / 2 - 1)));
	}

	_idx = 0;
}

void QrssPiG::_computeFft() {
	using namespace std::chrono;

	auto tt = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	int l = (tt  * int(_linesPerSecond) / 1000) % _frameSize;
	int f = tt / 1000 / _secondsPerFrame;

	_applyFilter();
	_fft->process();

	if ((_lastLine > 0) && (_lastLine != l)) {
		_fft->average();
		_im->drawLine(_fftOut, l);
		_fft->reset();

		if ((_lastFrame > 0) && (_lastFrame != f)) _pushImage();
		_pushImage();
		_lastFrame = f;
	}
	_lastLine = l;
}

void QrssPiG::_applyFilter() {
	for (int i = 0; i < _N; i++)_fftIn[i] *= _hannW[i/2];
}

void QrssPiG::_pushImage() {
	std::string s("test");
	int p = 0;

	try {
		_im->save2Buffer();
		if (_up) _up->pushFile(s + std::to_string(p) + ".png", _im->getBuffer(), _im->getBufferSize());
	} catch (const std::exception &e) {
		std::cerr << "Error pushing file: " << e.what() << std::endl;
	}
	p++;
}
