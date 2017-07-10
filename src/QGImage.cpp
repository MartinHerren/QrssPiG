#include "QGImage.h"

#include <iomanip>
#include <string>
#include <math.h>

QGImage::QGImage(int size, int sampleRate, int N): _size(size), _sampleRate(sampleRate), N(N) {
	_im = gdImageCreateTrueColor(100 + N, 10 + _size + 105); // TODO: configurable margins or at least some defines

	_imBuffer = nullptr;
	_imBufferSize = 0;

	_cd = 256;
	_c = new int[_cd];

	_dBmin = -20.;
	_dBmax = -0.;
	_dBdelta = _dBmax - _dBmin;

	// Allocate colormap, taken from 'qrx' colormap from RFAnalyzer, reduced to 256 palette due to use of libgd
	int ii = 0;
	for (int i = 0; i <= 255; i += 4) _c[ii++] = gdImageColorAllocate(_im, 0, 0, i);
	for (int i = 0; i <= 255; i += 4) _c[ii++] = gdImageColorAllocate(_im, 0, i, 255);
	for (int i = 0; i <= 255; i += 4) _c[ii++] = gdImageColorAllocate(_im, i, 255, 255 - i);
	for (int i = 0; i <= 255; i += 4) _c[ii++] = gdImageColorAllocate(_im, 255, 255 -  i, 0);

	//int bucket = (_sampleRate/100) / (_sampleRate/N);
	int bucket = N/(2*100);
	int white = gdTrueColor(255, 255, 255);

	for (int i = 0; i < N/2; i += bucket) {
		gdImageLine(_im, 100 + N/2 - i*bucket, 0, 100 + N/2 - i*bucket, 10, white);
		gdImageLine(_im, 100 + N/2 + i*bucket, 0, 100 + N/2 + i*bucket, 10, white);
	}

	// Render graph scale (hard coded to full range -100dB-0dB)
	// Tick-markers with dB indication
	std::string font = "ttf-dejavu/DejaVuSans.ttf";
	std::stringstream text;

	for (int i = -100; i <= 0; i += 10) {
		int x = 90;
		int y = 10 + _size - i;

		gdImageLine(_im, x, y, x + 3, y, white);

		text.str("");
		text.clear();
		text << i << "dB";

		// Calculate text's bounding box
		int brect[8];
		gdImageStringFT(nullptr, brect, white, (char *)font.c_str(), 8, 0, x, y, (char *)text.str().c_str());

		// Fix position according to bounding box to be nicely aligned with tick-markers
		x += x - brect[0] - (brect[2] - brect[0]) - 5; // right aligned with 5px space to tick-marker
		y += y - brect[1] + .5 * (brect[1] - brect[7]); // vertically centered on tick-marker
		gdImageStringFT(_im, brect, white, (char *)font.c_str(), 8, 0, x, y, (char *)text.str().c_str());
	}

	// Color bar
	for (double i = -100.; i <= 0.; i++) {
		int c = db2Color(i);
		gdImageSetPixel(_im, 95, 10 + _size - i, c);
		gdImageSetPixel(_im, 96, 10 + _size - i, c);
		gdImageSetPixel(_im, 97, 10 + _size - i, c);
		gdImageSetPixel(_im, 98, 10 + _size - i, c);
	}

	// Indicator for dBmin-max range
	gdImageLine(_im, 94, 10 + _size - _dBmin, 94, 10 + _size - _dBmax, white);
}

QGImage::~QGImage() {
	delete [] _c;
	if (_imBuffer) gdFree(_imBuffer);
	gdImageDestroy(_im);
}

void QGImage::drawLine(const std::complex<double> *fft, int lineNumber) {
	int whiteA = gdTrueColorAlpha(255, 255, 255, 125);

	for (int i = 1; i < N; i++) {
		gdImageLine(_im, 100 + (i + N/2) % N - 1, 10 + _size - 10 * log10(abs(fft[i - 1]) / N), 100 + (i + N/2) % N, 10 + _size - 10 * log10(abs(fft[i]) / N), whiteA);
	}

	for (int i = 2; i < N - 2; i++) {
		// Get normalized value with DC centered
		gdImageSetPixel(_im, 100 + (i + N/2) % N, 10 + lineNumber, db2Color(10 * log10(abs(fft[i]) / N)));
	}
}

void QGImage::save2Buffer() {
	if (_imBuffer) gdFree(_imBuffer);

	_imBuffer = (char *)gdImagePngPtr(_im, &_imBufferSize);
}

void QGImage::save(const std::string &fileName) {
	FILE *pngout;

	pngout = fopen(fileName.c_str(), "wb");
	gdImagePng(_im, pngout);
	fclose(pngout);
}

int QGImage::db2Color(double v) {
	if (v < _dBmin) v = _dBmin;
	if (v > _dBmax) v = _dBmax;

	return _c[(int)trunc((_cd - 1) * (v - _dBmin) / _dBdelta)];
}
