#include "QGImage.h"

#include <iomanip>
#include <string>
#include <math.h>

QGImage::QGImage(int size, int sampleRate, int N, Orientation orientation): _size(size), _sampleRate(sampleRate), N(N), _orientation(orientation) {
	// Define font
	_font = "ttf-dejavu/DejaVuSans.ttf";
	_fontSize = 8;

	// Calculate max size for frequency and dB labels
	int brect[8];
	gdImageStringFT(nullptr, brect, 0, (char *)_font.c_str(), _fontSize, 0, 0, 0, (char *)"000000000Hz");
	_freqLabelWidth = brect[2] - brect[0];
	_freqLabelHeight = brect[1] - brect[7];
	gdImageStringFT(nullptr, brect, 0, (char *)_font.c_str(), _fontSize, 0, 0, 0, (char *)"-100dB");
	_dBLabelWidth = brect[2] - brect[0];
	_dBLabelHeight = brect[1] - brect[7];

	// Allocate canvas
	switch (_orientation) {
	case Orientation::Horizontal:
		_im = gdImageCreateTrueColor(_freqLabelWidth + 10 + _size + 100 + 10 +_freqLabelWidth, N + 10 + _dBLabelHeight);
		break;

	case Orientation::Vertical:
		_im = gdImageCreateTrueColor(_dBLabelWidth + 10 + N, _freqLabelHeight + 10 + _size + 100 + 10 + _freqLabelHeight);
		break;
	}

	_imBuffer = nullptr;
	_imBufferSize = 0;

	// Define colormap
	_cd = 256;
	_c = new int[_cd];

	// Allocate colormap, taken from 'qrx' colormap from RFAnalyzer, reduced to 256 palette due to use of libgd
	int ii = 0;
	for (int i = 0; i <= 255; i += 4) _c[ii++] = gdImageColorAllocate(_im, 0, 0, i);
	for (int i = 0; i <= 255; i += 4) _c[ii++] = gdImageColorAllocate(_im, 0, i, 255);
	for (int i = 0; i <= 255; i += 4) _c[ii++] = gdImageColorAllocate(_im, i, 255, 255 - i);
	for (int i = 0; i <= 255; i += 4) _c[ii++] = gdImageColorAllocate(_im, 255, 255 -  i, 0);

	_drawFreqScale();
	setScale(-30., 0.);
	clearGraph();
}

QGImage::~QGImage() {
	delete [] _c;
	if (_imBuffer) gdFree(_imBuffer);
	gdImageDestroy(_im);
}

void QGImage::setScale(double dBmin, double dBmax) {
	_dBmin = dBmin;
	_dBmax = dBmax;
	_dBdelta = _dBmax - _dBmin;

	_drawDbScale();
}

void QGImage::clearGraph() {
	switch (_orientation) {
	case Orientation::Horizontal:
		gdImageFilledRectangle(_im, _freqLabelWidth + 10, N, _freqLabelWidth + 10 + _size + 100, 0, gdTrueColor(0, 0, 0));
		break;

	case Orientation::Vertical:
		gdImageFilledRectangle(_im, _dBLabelWidth + 10, _freqLabelHeight + 10 + _size + 100, _dBLabelWidth + 10 + N, _freqLabelHeight + 10, gdTrueColor(0, 0, 0));
		break;
	}
}

void QGImage::drawLine(const std::complex<double> *fft, int lineNumber) {
	if (lineNumber >= _size) return;
	int whiteA = gdTrueColorAlpha(255, 255, 255, 125);

	// Draw a data line DC centered
	double last;

	for (int i = 0; i < N; i++) {
		double v = 10 * log10(abs(fft[(i + N/2) % N]) / N); // Current value, DC centered

		switch (_orientation) {
		case Orientation::Horizontal:
			gdImageSetPixel(_im, _freqLabelWidth + 10 + lineNumber, N - i, _db2Color(v));
			if (i > 0) gdImageLine(_im, _freqLabelWidth + 10 + _size - last, N - i + 1, _freqLabelWidth + 10 + _size - v, N - i, whiteA);
			break;

		case Orientation::Vertical:
			gdImageSetPixel(_im, _dBLabelWidth + 10 + i, _freqLabelHeight + 10 + lineNumber, _db2Color(v));
			if (i > 0) gdImageLine(_im, _dBLabelWidth + 10 + i - 1, _freqLabelHeight + 10 + _size - last, _dBLabelWidth + 10 + i, _freqLabelHeight + 10 + _size - v, whiteA);
			break;
		}

		last = v;
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

// Private members

void QGImage::_drawFreqScale() {
	int white = gdTrueColor(255, 255, 255);
	//int bucket = (_sampleRate/100) / (_sampleRate/N);
	//int bucket = N/(2*100);
	int bucket = 10;

	for (int i = 0; i < N; i += bucket) {
		std::stringstream f;
		f << (((i - N/2) * _sampleRate) / N) << "Hz";

		// Calculate text's bounding box
		int brect[8];
		gdImageStringFT(nullptr, brect, white, (char *)_font.c_str(), _fontSize, 0, 0, 0, (char *)f.str().c_str());
		int y = brect[1], w = brect[2] - brect[0], h = brect[1] - brect[7];

		switch (_orientation) {
		case Orientation::Horizontal:
			gdImageLine(_im, _freqLabelWidth + 1, N - i, _freqLabelWidth + 9, N - i, white);
			gdImageStringFT(_im, brect, white, (char *)_font.c_str(), _fontSize, 0, _freqLabelWidth - w, N - i - y + .5 * h, (char *)f.str().c_str());
			gdImageLine(_im, _freqLabelWidth + 10 + _size + 100 + 1, N - i, _freqLabelWidth + 10 + _size + 100 + 9, N - i, white);
			gdImageStringFT(_im, brect, white, (char *)_font.c_str(), _fontSize, 0, _freqLabelWidth + 10 + _size + 100 + 10, N - i - y + .5 * h, (char *)f.str().c_str());
			break;

		case Orientation::Vertical:
			gdImageLine(_im, _dBLabelWidth + 10 + N - i, _freqLabelHeight + 1, _dBLabelWidth + 10 + N - i, _freqLabelHeight + 9, white);
			gdImageLine(_im, _dBLabelWidth + 10 + N - i, _freqLabelHeight + 10 + _size + 100 + 1, _dBLabelWidth + 10 + N - i, _freqLabelHeight + 10 + _size + 100 + 9, white);
			break;
		}
	}

                // Fix position according to bounding box to be nicely aligned with tick-markers
                //int xT = 90 - brect[0] - (brect[2] - brect[0]) - 5; // right aligned with 5px space to tick-marker
                //int yT = 10 + _size - i - brect[1] + .5 * (brect[1] - brect[7]); // vertically centered on tick-marker
                //gdImageStringFT(_im, brect, white, (char *)_font.c_str(), 8, 0, xT, yT, (char *)text.str().c_str());
}

void QGImage::_drawTimeScale() {
}

void QGImage::_drawDbScale() {
	switch (_orientation) {
	case Orientation::Horizontal:
		gdImageFilledRectangle(_im, _freqLabelWidth + 10 + _size, N + 10 + _dBLabelHeight, _freqLabelWidth + 10 + _size + 100, N, gdTrueColor(0, 0, 0));
		break;

	case Orientation::Vertical:
		gdImageFilledRectangle(_im, 0, _freqLabelHeight + 10 + _size + 100, _dBLabelWidth + 10, _freqLabelHeight + 10 + _size, gdTrueColor(0, 0, 0));
		break;
	}

	int white = gdTrueColor(255, 255, 255);

	// Render graph scale (hard coded to full range -100dB-0dB)
	// Tick-markers with dB indication

	for (int i = -100; i <= 0; i += 10) {
		std::stringstream text;
		text << i << "dB";

		// Calculate text's bounding box
		int brect[8];
		gdImageStringFT(nullptr, brect, white, (char *)_font.c_str(), 8, 0, 0, 0, (char *)text.str().c_str());

		// Fix position according to bounding box to be nicely aligned with tick-markers
		int x, y;
		switch (_orientation) {
		case Orientation::Horizontal:
			x = _dBLabelWidth + 10 + _size - i - 0.5 * (brect[2] - brect[0]); // right aligned to tick-marker
			y = N + 10 - brect[1] + brect[1] - brect[7]; // vertically centered on tick-marker
			gdImageStringFT(_im, brect, white, (char *)_font.c_str(), 8, 0, x, y, (char *)text.str().c_str());
			break;

		case Orientation::Vertical:
			x = _dBLabelWidth - brect[0] - (brect[2] - brect[0]); // right aligned to tick-marker
			y = _freqLabelHeight + 10 + _size - i - brect[1] + .5 * (brect[1] - brect[7]); // vertically centered on tick-marker
			gdImageStringFT(_im, brect, white, (char *)_font.c_str(), 8, 0, x, y, (char *)text.str().c_str());
			break;
		}

		switch (_orientation) {
		case Orientation::Horizontal:
			gdImageLine(_im, _freqLabelWidth + 10 + _size - i, N + 10, _freqLabelWidth + 10 + _size - i, N + 7, white);
			break;

		case Orientation::Vertical:
			gdImageLine(_im, _dBLabelWidth, _freqLabelHeight + 10 + _size - i, _dBLabelWidth + 3, _freqLabelHeight + 10 + _size - i, white);
			break;
		}
	}

	// Indicator for dBmin-max range
	switch (_orientation) {
	case Orientation::Horizontal:
		gdImageLine(_im, _freqLabelWidth + 10 + _size - _dBmin, N + 6, _freqLabelWidth + 10 + _size - _dBmax, N + 6, white);
		break;

	case Orientation::Vertical:
		gdImageLine(_im, _dBLabelWidth + 4, _freqLabelHeight + 10 + _size - _dBmin, _dBLabelWidth + 4, _freqLabelHeight + 10 + _size - _dBmax, white);
		break;
	}

	// Color bar
	for (double i = -100.; i <= 0.; i++) {
		int c = _db2Color(i);

		switch (_orientation) {
		case Orientation::Horizontal:
			gdImageSetPixel(_im, _freqLabelWidth + 10 + _size - i, N + 2, c);
			gdImageSetPixel(_im, _freqLabelWidth + 10 + _size - i, N + 3, c);
			gdImageSetPixel(_im, _freqLabelWidth + 10 + _size - i, N + 4, c);
			gdImageSetPixel(_im, _freqLabelWidth + 10 + _size - i, N + 5, c);
			break;

		case Orientation::Vertical:
			gdImageSetPixel(_im, _dBLabelWidth + 5, _freqLabelHeight + 10 + _size - i, c);
			gdImageSetPixel(_im, _dBLabelWidth + 6, _freqLabelHeight + 10 + _size - i, c);
			gdImageSetPixel(_im, _dBLabelWidth + 7, _freqLabelHeight + 10 + _size - i, c);
			gdImageSetPixel(_im, _dBLabelWidth + 8, _freqLabelHeight + 10 + _size - i, c);
			break;
		}
	}
}

int QGImage::_db2Color(double v) {
	if (v < _dBmin) v = _dBmin;
	if (v > _dBmax) v = _dBmax;

	return _c[(int)trunc((_cd - 1) * (v - _dBmin) / _dBdelta)];
}
