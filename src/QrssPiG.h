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

	void fft(int y);
	void push();

private:
	bool _unsignedIQ;
	int _sampleRate;
	int _N; // FFT size

	QGFft *_fft;

public:
	std::complex<double> *_fftIn;
	std::complex<double> *_fftOut;

private:
	QGImage *_im;
	QGUploader *_up;
};
