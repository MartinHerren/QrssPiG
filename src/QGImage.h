#pragma once

#include <iostream>

#include <complex> // Must be included before fftw3
#include <fftw3.h>
#include <gd.h>

class QGImage {
public:
	enum class Orientation { Horizontal, Vertical };

	QGImage(int size, int sampleRate, int N, Orientation orientation = Orientation::Horizontal);
	~QGImage();

	void setScale(double dBmin, double dBmax);
	void clearGraph();

	void drawLine(const std::complex<double> *fft, int lineNumber);
	void save2Buffer();
	void save(const std::string &fileName);

	char *getBuffer() { return _imBuffer; };
	int getBufferSize() { return _imBufferSize;} ;

private:
	void _drawFreqScale();
	void _drawTimeScale();
	void _drawDbScale();

	int _db2Color(double v);

	int _size;
	int _sampleRate;
	int N;
	Orientation _orientation;

	gdImagePtr _im;
	char *_imBuffer;
	int _imBufferSize;
	int *_c;
	int _cd; // Color depth

	std::string _font;
	int _fontSize;
	int _freqLabelWidth;
	int _freqLabelHeight;
	int _dBLabelWidth;
	int _dBLabelHeight;

	// Covered dB range. Only used for waterfall colormapping, not in spectro graph yet
	double _dBmin;
	double _dBmax;
	double _dBdelta;

	// Covered frequency range. In DC centered 'fft' bucket, 0 means DC, N/2 means max positive frequency
	// Negative value make only sense with IQ data, not audio data
	int _fMin;
	int _fMax;
	int _fDelta;
};
