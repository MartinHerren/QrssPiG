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
	Status addLine(const std::complex<float> *fft);
	char *getFrame(int *frameSize, std::string &frameName);

private:
	void _init();
	void _free();

	void _computeFreqScale();
	void _drawFreqScale();
	void _computeDbScale();
	void _drawDbScale();
	void _computeTimeScale();
	void _drawTimeScale();

	int _db2Color(float v);

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
	float _dBmin;
	float _dBmax;
	float _dBdelta;

	// Frame alignement and start
	bool _alignFrame;
	bool _syncFrames;
	std::chrono::milliseconds _started;
	std::chrono::milliseconds _startedIntoFrame;

	// Internal data
	gdImagePtr _im;
	char *_imBuffer;
	int *_c;
	int _cd; // Color depth
	int _currentLine;

	// Constants to go from hertz/seconds to pixel
	float _freqK;
	float _dBK;
	float _timeK;

	// Marker and label division
	int _hertzPerFreqLabel;
	int _freqLabelDivs;
	int _dBPerDbLabel;
	int _dBLabelDivs;
	int _secondsPerTimeLabel;
	int _timeLabelDivs;

	// Max size of markes and labels to allocate canvas
	std::string _qrsspigString;
	int _qrsspigLabelWidth;
	int _qrsspigLabelHeight;

	int _borderSize;
	int _titleHeight;
	int _scopeSize;
	int _scopeRange;
	int _markerSize;
	int _freqLabelWidth;
	int _freqLabelHeight;
	int _dBLabelWidth;
	int _dBLabelHeight;
	int _timeLabelWidth;
	int _timeLabelHeight;
};
