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

	// Calculate _fMin/Max/Delta from frequencies
	_fMin = (0 * N) / _sampleRate;
	_fMax = (2500 * N) / _sampleRate;
	_fDelta = _fMax - _fMin;

	// Allocate canvas
	switch (_orientation) {
	case Orientation::Horizontal:
		_im = gdImageCreateTrueColor(_freqLabelWidth + 10 + _size + 100 + 10 +_freqLabelWidth, _fDelta + 10 + _dBLabelHeight);
		break;

	case Orientation::Vertical:
		_im = gdImageCreateTrueColor(_dBLabelWidth + 10 + _fDelta, _freqLabelHeight + 10 + _size + 100 + 10 + _freqLabelHeight);
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
		gdImageFilledRectangle(_im, _freqLabelWidth + 10, _fDelta, _freqLabelWidth + 10 + _size + 100, 0, gdTrueColor(0, 0, 0));
		break;

	case Orientation::Vertical:
		gdImageFilledRectangle(_im, _dBLabelWidth + 10, _freqLabelHeight + 10 + _size + 100, _dBLabelWidth + 10 + _fDelta, _freqLabelHeight + 10, gdTrueColor(0, 0, 0));
		break;
	}
}

void QGImage::drawLine(const std::complex<double> *fft, int lineNumber) {
	if (lineNumber >= _size) return;
	int whiteA = gdTrueColorAlpha(255, 255, 255, 125);

	// Draw a data line DC centered
	double last;

	for (int i = _fMin; i < _fMax; i++) {
		double v = 10 * log10(abs(fft[(i + N) % N]) / N); // Current value, DC centered

		switch (_orientation) {
		case Orientation::Horizontal:
			gdImageSetPixel(_im, _freqLabelWidth + 10 + lineNumber, _fDelta + _fMin - i, _db2Color(v));
			if (i != _fMin) gdImageLine(_im, _freqLabelWidth + 10 + _size - last, _fDelta + _fMin - i + 1, _freqLabelWidth + 10 + _size - v, _fDelta + _fMin - i, whiteA);
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
	int topLeftX, topLeftY, topLeftX2, topLeftY2;
	int tickStep, labelStep;
	int white = gdTrueColor(255, 255, 255);

	// Clear zones and set topLeft corners
	if (_orientation == Orientation::Horizontal) {
		topLeftX = 0;
		topLeftY = 0;
		topLeftX2 = _freqLabelWidth + 10 + _size + 100;
		topLeftY2 = 0;
		tickStep = 10;
		labelStep = tickStep * 10;
		gdImageFilledRectangle(_im, topLeftX, topLeftY, topLeftX + _freqLabelWidth + 10, topLeftY + _fDelta, gdTrueColor(0, 0, 0));
		gdImageFilledRectangle(_im, topLeftX2, topLeftY2, topLeftX2 + 10 + _freqLabelWidth, topLeftY2 + _fDelta, gdTrueColor(0, 0, 0));
	} else {
		topLeftX = _dBLabelWidth + 10;
		topLeftY = 0;
		topLeftX2 = _dBLabelWidth + 10;
		topLeftY2 = _freqLabelHeight + 10 + _size + 100;
		tickStep = 10;
		labelStep = tickStep * 10;
		gdImageFilledRectangle(_im, topLeftX, topLeftY, topLeftX + _fDelta, topLeftY + _freqLabelWidth + 10, gdTrueColor(0, 0, 0));
		gdImageFilledRectangle(_im, topLeftX2, topLeftY2, topLeftX2 + _fDelta, topLeftY2 + _freqLabelWidth + 10, gdTrueColor(0, 0, 0));
	}

	// Freq labels
	for (int i = _fMin; i < _fMax; i += labelStep) {
		std::stringstream f;
		f << ((i * _sampleRate) / N) << "Hz";

		// Calculate text's bounding box
		int brect[8];
		gdImageStringFT(nullptr, brect, white, (char *)_font.c_str(), _fontSize, 0, 0, 0, (char *)f.str().c_str());

		// Cache key data as they will be overriden when rendering first string
		int x = brect[0], y = brect[1], w = brect[2] - brect[0], h = brect[1] - brect[7];

		if (_orientation == Orientation::Horizontal) {
			gdImageStringFT(_im, brect, white, (char *)_font.c_str(), _fontSize, 0, topLeftX + _freqLabelWidth - w, topLeftY + _fDelta + _fMin - i - y + .5 * h, (char *)f.str().c_str());
			gdImageStringFT(_im, brect, white, (char *)_font.c_str(), _fontSize, 0, topLeftX2 + 10, topLeftY2 + _fDelta + _fMin - i - y + .5 * h, (char *)f.str().c_str());
		} else {
			gdImageStringFT(_im, brect, white, (char *)_font.c_str(), _fontSize, 0, topLeftX + i - w/2, topLeftY + _freqLabelHeight - x, (char *)f.str().c_str());
			gdImageStringFT(_im, brect, white, (char *)_font.c_str(), _fontSize, 0, topLeftX2 + i - w/2, topLeftY2 + 10 + _freqLabelHeight - x, (char *)f.str().c_str());
		}
	}

	// Tick markersA
	for (int i = _fMin; i < _fMax; i += tickStep) {
		if (_orientation == Orientation::Horizontal) {
			gdImageLine(_im, topLeftX + _freqLabelWidth + 1, topLeftY + _fDelta + _fMin - i, topLeftX + _freqLabelWidth + 9, topLeftY + _fDelta + _fMin - i, white);
			gdImageLine(_im, topLeftX2 + 1, topLeftY2 + _fDelta + _fMin - i, topLeftX2 + 9, topLeftY2 + _fDelta + _fMin - i, white);
		} else {
			gdImageLine(_im, topLeftX + i, _freqLabelHeight + 1, topLeftX + i, _freqLabelHeight + 9, white);
			gdImageLine(_im, topLeftX2 + i, _freqLabelHeight + 10 + _size + 100 + 1, topLeftX2 + i, _freqLabelHeight + 10 + _size + 100 + 9, white);
		}
	}
}

void QGImage::_drawTimeScale() {
}

void QGImage::_drawDbScale() {
	int topLeftX, topLeftY;
	int tickStep, labelStep;
	int white = gdTrueColor(255, 255, 255);

	// Clear zone and set topLeft corner
	if (_orientation == Orientation::Horizontal) {
		topLeftX = _freqLabelWidth + 10 + _size;
		topLeftY = _fDelta;
		tickStep = 10;
		labelStep = tickStep * 3;
		gdImageFilledRectangle(_im, topLeftX, topLeftY, topLeftX + 100, topLeftY + 10 + _dBLabelHeight, gdTrueColor(0, 0, 0));
	} else {
		topLeftX = 0;
		topLeftY = _freqLabelHeight + 10 + _size;
		tickStep = 10;
		labelStep = tickStep;
		gdImageFilledRectangle(_im, topLeftX, topLeftY, topLeftX + _dBLabelWidth + 10, topLeftY + 100, gdTrueColor(0, 0, 0));
	}

	// Render graph scale (hard coded to full range -100dB-0dB)

	// dB labels
	for (int i = 0; i >= -100; i -= labelStep) {
		std::stringstream text;
		text << i << "dB";

		// Calculate text's bounding box
		int brect[8];
		gdImageStringFT(nullptr, brect, white, (char *)_font.c_str(), 8, 0, 0, 0, (char *)text.str().c_str());

		// Fix position according to bounding box to be nicely aligned with tick-markers
		int x, y;

		if (_orientation == Orientation::Horizontal) {
			x = topLeftX - i - 0.5 * (brect[2] - brect[0]);
			y = topLeftY + 10 - brect[7];
		} else {
			x = topLeftX +_dBLabelWidth - brect[2];
			y = topLeftY - i - .5 * (brect[1] + brect[7]);
		}

		gdImageStringFT(_im, brect, white, (char *)_font.c_str(), 8, 0, x, y, (char *)text.str().c_str());
	}

	// Tick markers
	for (int i = 0; i >= -100; i -= tickStep) {
		if (_orientation == Orientation::Horizontal)
			gdImageLine(_im, topLeftX - i, topLeftY + 10, topLeftX - i, topLeftY + 7, white);
		else
			gdImageLine(_im, topLeftX + _dBLabelWidth, topLeftY - i, topLeftX + _dBLabelWidth + 3, topLeftY - i, white);
	}

	// Indicator bar for dBmin-max range
	if (_orientation == Orientation::Horizontal)
		gdImageLine(_im, topLeftX - _dBmin, topLeftY + 6, topLeftX - _dBmax, topLeftY + 6, white);
	else
		gdImageLine(_im, topLeftX + _dBLabelWidth + 4, topLeftY - _dBmin, topLeftX +_dBLabelWidth + 4, topLeftY - _dBmax, white);

	// Color bar
	for (double i = -100.; i <= 0.; i++) {
		int c = _db2Color(i);

		if (_orientation == Orientation::Horizontal)
			gdImageLine(_im, topLeftX - i, topLeftY + 2, topLeftX - i, topLeftY + 5, c);
		else
			gdImageLine(_im, topLeftX + _dBLabelWidth + 5, topLeftY - i, topLeftX +_dBLabelWidth + 8, topLeftY - i, c);
	}
}

int QGImage::_db2Color(double v) {
	if (v < _dBmin) v = _dBmin;
	if (v > _dBmax) v = _dBmax;

	return _c[(int)trunc((_cd - 1) * (v - _dBmin) / _dBdelta)];
}
