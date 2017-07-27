#pragma once

#include <chrono>
#include <complex> // Must be included before fftw3
#include <iostream>

#include <fftw3.h>
#include <gd.h>
#include <yaml-cpp/yaml.h>

class QGImage {
public:
	enum class Orientation { Horizontal, Vertical };

	QGImage(int sampleRate, int baseFreq, int fftSize, int fftOverlap);
	~QGImage();

	void configure(const YAML::Node &config);
	void startFrame(std::chrono::milliseconds startTime);

	void drawLine(const std::complex<double> *fft, int lineNumber);
	void save2Buffer();
	void save(const std::string &fileName);

	char *getBuffer() { return _imBuffer; };
	int getBufferSize() { return _imBufferSize;} ;

private:
	void _init();
	void _free();

	void _drawFreqScale();
	void _drawDbScale();
	void _drawTimeScale(std::chrono::milliseconds startTime);

	int _db2Color(double v);

	// Params given at constructor time, cannot be changed
	int _sampleRate;
	int _baseFreq;
	int N;
	int _overlap;

	// Configuration
	Orientation _orientation;
	int _size;

	std::string _font;
	int _fontSize;

	// Covered frequency range. In baseband DC centered 'fft' bucket, 0 means DC, N/2 means max positive frequency
	// Negative value make only sense with IQ data, not audio data
	int _fMin;
	int _fMax;
	int _fDelta;

	// Covered dB range. Only used for waterfall colormapping, not in spectro graph yet
	double _dBmin;
	double _dBmax;
	double _dBdelta;

	// Internal data
	gdImagePtr _im;
	char *_imBuffer;
	int _imBufferSize;
	int *_c;
	int _cd; // Color depth

	int _freqLabelWidth;
	int _freqLabelHeight;
	int _dBLabelWidth;
	int _dBLabelHeight;
};
