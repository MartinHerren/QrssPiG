#include "QGImage.h"

#include <math.h>

QGImage::QGImage(int size, int sampleRate, int N): _size(size), _sampleRate(sampleRate), N(N) {
	_im = gdImageCreate(N, 10 + _size + 100);

	_imBuffer = nullptr;
	_imBufferSize = 0;

	_cd = 256;
	_c = new int[_cd];

	// Allocate colormap, taken from 'qrx' colormap from RFAnalyzer, reduced to 256 palette due to use of libgd
	int ii = 0;
	for (int i = 0; i <= 255; i += 4) _c[ii++] = gdImageColorAllocate(_im, 0, 0, i);
	for (int i = 0; i <= 255; i += 4) _c[ii++] = gdImageColorAllocate(_im, 0, i, 255);
	for (int i = 0; i <= 255; i += 4) _c[ii++] = gdImageColorAllocate(_im, i, 255, 255 - i);
	for (int i = 0; i <= 255; i += 4) _c[ii++] = gdImageColorAllocate(_im, 255, 255 -  i, 0);

	//int bucket = (_sampleRate/100) / (_sampleRate/N);
	int bucket = N/(2*100);

	//int black = gdImageColorAllocate(_im, 0, 0, 0);
	//int white = gdImageColorAllocate(_im, 255, 255, 255);

	//gdImageFilledRectangle(_im, 0, 0, N, 10, black);

	for (int i = 0; i < N/2; i += bucket) {
		gdImageLine(_im, N/2 - i*bucket, 0, N/2 - i*bucket, 10, _c[_cd-1]);
		gdImageLine(_im, N/2 + i*bucket, 0, N/2 + i*bucket, 10, _c[_cd-1]);
	}
}

QGImage::~QGImage() {
	delete [] _c;
	if (_imBuffer) gdFree(_imBuffer);
	gdImageDestroy(_im);
}

void QGImage::drawLine(const std::complex<double> *fft, int lineNumber) {
	// Set min range
	double min = -50, max = -40;

	for (int i = 1; i < N; i++) {
		gdImageLine(_im, (i + N/2) % N - 1, 10 * log10(abs(fft[i - 1]) / N) + 10 + _size + 100, (i + N/2) % N, 10 * log10(abs(fft[i]) / N) + 10 + _size + 100, _c[_cd-1]);
	}

	// Get extrem values
	for (int i = 2; i < N - 2; i++) {
		double n = 10 * log10(abs(fft[i]) / N); // TODO: search min max without log10, do log10 on found values
		if (n > max) max = n;
		if (n < min) min = n;
	}

	// Clip to max range
	if (min < -100) min = -100;
	if (max > 0) max = 0;

	double delta = max - min;

	for (int i = 2; i < N - 2; i++) {
		// Get normalized value with DC centered
		double n = 10 * log10(abs(fft[i]) / N);

		// Clip to min-max interval
		if (n < min) n = min;
		if (n > max) n = max;

		int v = (int)trunc((_cd - 1) * (n - min) / delta);
		// Safety clip, should no happen
		if (v < 0) v = 0;
		if (v >= _cd) v = _cd - 1;

		gdImageSetPixel(_im, (i + N/2) % N, 10 + lineNumber, _c[v]);
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
