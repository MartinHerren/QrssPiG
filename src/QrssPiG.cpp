#include "QrssPiG.h"

#include <chrono>
#include <string>

#include <yaml-cpp/yaml.h>

QrssPiG::QrssPiG(bool unsignedIQ, int sampleRate, int N) :
	_unsignedIQ(unsignedIQ),
	_sampleRate(sampleRate),
	_N(N) {
	_fft = new QGFft(N);
	_fftIn = _fft->getInputBuffer();
	_fftOut = _fft->getFftBuffer();

	_secondsPerFrame = 60;
	_frameSize = 240;
	_linesPerSecond = (double)_frameSize / _secondsPerFrame;

	_lastLine = -1;
	_lastFrame = -1;

	_im = new QGImage(_frameSize, _sampleRate, _N);
	_up = nullptr;
}

QrssPiG::~QrssPiG() {
	_push();

	if (_fft) delete _fft;
	if (_im) delete _im;
	if (_up) delete _up;
}

void QrssPiG::addUploader(const std::string &sshHost, const std::string &sshUser, const std::string &sshDir, int sshPort) {
	if (_up) delete _up;
	_up = new QGUploader(sshHost, sshUser, sshDir, sshPort);
}

void QrssPiG::fft() {
	using namespace std::chrono;

	auto tt = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	int l = (tt  * int(_linesPerSecond) / 1000) % _frameSize;
	int f = tt / 1000 / _secondsPerFrame;

	_fft->process();

	if ((_lastLine > 0) && (_lastLine != l)) {
		_fft->average();
		_im->drawLine(_fftOut, l);
		_fft->reset();

		if ((_lastFrame > 0) && (_lastFrame != f)) _push();
		_push();
		_lastFrame = f;
	}
	_lastLine = l;
}

void QrssPiG::_push() {
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
