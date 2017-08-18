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
	enum class Status { Ok, FrameReady };

	QGImage(long int sampleRate, long int baseFreq, int fftSize, int fftOverlap);
	~QGImage();

	void configure(const YAML::Node &config);

	void startNewFrame(bool incrementTime = true);
	Status addLine(const std::complex<double> *fft);
	char *getFrame(int *frameSize, std::string &frameName);

private:
	void _init();
	void _free();

	void _drawFreqScale();
	void _drawDbScale();
	void _computeTimeScale();
	void _drawTimeScale();

	int _db2Color(double v);

	// Params given at constructor time, cannot be changed
	long int _sampleRate;
	long int _baseFreq;
	int N;
	int _overlap;

	// Configuration
	Orientation _orientation;
	int _secondsPerFrame;
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
	int *_c;
	int _cd; // Color depth
	std::chrono::milliseconds _started; // current frame start
	int _currentLine;

	double _timeK;

	std::string _qrsspigString;
	int _qrsspigLabelBase;
	int _qrsspigLabelWidth;
	int _qrsspigLabelHeight;

	int _secondsPerTimeLabel;
	int _timeLabelDivs;

	int _freqLabelWidth;
	int _freqLabelHeight;
	int _dBLabelWidth;
	int _dBLabelHeight;
};
