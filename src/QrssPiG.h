#include <complex>
#include <iostream>
#include <string>

#include "QGFft.h"
#include "QGImage.h"
#include "QGUploader.h"

class QrssPiG {
public:
	QrssPiG(bool unsignedIQ, int sampleRate, int N = 2048);
	~QrssPiG();

	void addUploader(const std::string &sshHost, const std::string &sshUser, const std::string &sshDir, int sshPort);

	void fft();

private:
	void _push();

private:
	// Data format
	bool _unsignedIQ;
	int _sampleRate;

	// Image format
	int _secondsPerFrame;
	int _frameSize;
	double _linesPerSecond;

	int _N; // FFT size

	QGFft *_fft;

public:
	std::complex<double> *_fftIn;
	std::complex<double> *_fftOut;

private:
	QGImage *_im;
	QGUploader *_up;

	int _lastLine;
	int _lastFrame;
};
