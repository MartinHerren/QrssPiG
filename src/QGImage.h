#pragma once

#include <chrono>
#include <complex>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <gd.h>
#include <yaml-cpp/yaml.h>

class QGImage {
public:
	QGImage(const YAML::Node &config, unsigned int index);
	~QGImage();

	void addCb(std::function<void(const std::string&, const std::string&, long int, std::chrono::milliseconds, const char*, int, bool, bool)>cb);

	void addLine(const std::complex<float> *fft);

private:
	enum class Orientation { Horizontal, Vertical };
	enum class Format { PNG, JPG };

	void _init();
	void _new(bool incrementTime = true);
	void _free();

	void _computeTitleHeight();
	void _addSubTitleField(std::string field, bool newline = false);
	void _renderTitle();
	void _computeFreqScale();
	void _drawFreqScale();
	void _computeDbScale();
	void _drawDbScale();
	void _computeTimeScale();
	void _drawTimeScale();

	int _db2Color(float v);

	void _pushFrame(bool intermediate = false, bool wait = false);

	std::string _levelBar(float v);

	// Params given at constructor time, cannot be changed
	int N;
	int _overlap;

	// Configuration
	long int _sampleRate;
	long int _baseFreq;
	long int _baseFreqCorrected;

	std::string _fileNameTmpl;
	std::string _fileNameExt;
	Format _format;

	std::string _title;
	std::vector<std::string> _subtitles;
	std::string _inputType;
	long int _inputSampleRate;
	std::string _callsign;
	std::string _qth;
	std::string _receiver;
	std::string _antenna;

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
	std::chrono::milliseconds _runningSince;
	std::chrono::milliseconds _startedIntoFrame;

	bool _levelMeter;

	// Internal data
	gdImagePtr _im;
	char *_imBuffer;
	std::unique_ptr<int[]> _c;
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

	std::vector<std::function<void(const std::string&, const std::string&, long int, std::chrono::milliseconds, const char*, int, bool, bool)>> _cbs;
};
