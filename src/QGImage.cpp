#include "QGImage.h"

#include "math.h"

QGImage::QGImage(int N): N(N), _cd(128) {
	_im = gdImageCreate(N, 2000);
	
	_c = new int[_cd + 1];
	
	for (int i = 0; i <= _cd; i++) {
		_c[i] = gdImageColorAllocate(_im, i * 255 / _cd, 0, (_cd-i) * 255 / _cd);
	}
}

QGImage::~QGImage() {
	delete [] _c;
	gdImageDestroy(_im);
}

void QGImage::drawLine(const std::complex<double> *fft, const int lineNumber) {
	int v;
	
	for (int i = 1; i < N; i++) {
		// Get normalized value with DC centered scaled to 32
		v = (int)trunc(_cd * abs(fft[(i + N/2) % N]) / N) * 10;
		
		// Clip maximum value
		if (v > _cd) v = _cd;
		
		gdImageSetPixel(_im, i, lineNumber, _c[v]);
	}
}

void QGImage::save(const std::string &fileName) {
	FILE *pngout;
	
	pngout = fopen(fileName.c_str(), "wb");
	gdImagePng(_im, pngout);
	fclose(pngout);
}
