#include "QGImage.h"

#include "math.h"

QGImage::QGImage(int sampleRate, int N): _sampleRate(sampleRate), N(N) {
	_im = gdImageCreate(N, 10 + 2000);

	_imBuffer = nullptr;
	_imBufferSize = 0;

	_cd = 256;
	_c = new int[_cd];

	// Allocate colormap, taken from 'qrx' colormap from RFAnalyzer, reduced to 256 palette due to use of libgd
	int ii = 0;
	for (int i = 0; i <= 255; i+=4,ii++) _c[ii] = gdImageColorAllocate(_im, 0, i, 255);
	for (int i = 0; i <= 255; i+=4,ii++) _c[ii] = gdImageColorAllocate(_im, 0, 255, 255 - i);
	for (int i = 0; i <= 255; i+=4,ii++) _c[ii] = gdImageColorAllocate(_im, i, 255, 0);
	for (int i = 0; i <= 255; i+=4,ii++) _c[ii] = gdImageColorAllocate(_im, 255, 255 -  i, 0);

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
	int v;

	double min = 1, max = 0;
	int maxv = 0;

	// Get extrem values
	for (int i = 1; i < N; i++) {
		double n = 10*log10(abs(fft[i]) / N);

		if (n > max) max = n;
		if (n < min) min = n;
	}

	double delta = max - min;

	for (int i = 1; i < N; i++) {
		// Get normalized value with DC centered
		double n = 10*log10(abs(fft[i]) / N);

		v = (int)trunc((_cd - 1) * (n - min) / delta);
		//		std::cout << n << ": " << v << std::endl;

		// Clip maximum value
		if (v >= _cd) v = _cd - 1;
		if (v > maxv) maxv = v;

		//		std::cout << v << std::endl;
		gdImageSetPixel(_im, (i + N/2) % N, 10 + lineNumber, _c[v]);
		//gdImageSetPixel(_im, i, 10 + lineNumber, _c[v]);
	}
	std::cout << min << " " << max << " " << maxv << std::endl;
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
