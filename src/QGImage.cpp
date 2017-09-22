#include "QGImage.h"
#include "Config.h"

#include <iomanip>
#include <stdexcept>
#include <string>
#include <math.h>

QGImage::QGImage(long int sampleRate, long int baseFreq, int fftSize, int fftOverlap): _sampleRate(sampleRate), _baseFreq(baseFreq), N(fftSize), _overlap(fftOverlap) {
	_im = nullptr;
	_imBuffer = nullptr;
	_c = nullptr;
	_cd = 0;
	_started = std::chrono::milliseconds(0);
	_currentLine = 0;

	// Freq/time constants used for mapping freq/time to pixel
	_freqK = (float)(N) / (_sampleRate); // pixels = freq * _freqK
	_timeK = (float)(_sampleRate) / (N - _overlap); // pixels = seconds * _timeK
}

QGImage::~QGImage() {
	_free();
}

void QGImage::configure(const YAML::Node &config) {
	_free();

	// Configure orientation
	_orientation = Orientation::Horizontal;
	if (config["orientation"]) {
		std::string o = config["orientation"].as<std::string>();

		if (o.compare("horizontal") == 0) _orientation = Orientation::Horizontal;
		else if (o.compare("vertical") == 0) _orientation = Orientation::Vertical;
		else throw std::runtime_error("QGImage::configure: output orientation unrecognized");
	}

	// Configure and calculate size
	_secondsPerFrame = 10 * 60;
	if (config["minutesperframe"]) _secondsPerFrame = config["minutesperframe"].as<int>() *  60;
	if (config["secondsperframe"]) _secondsPerFrame = config["secondsperframe"].as<int>();
	_size = _secondsPerFrame * _timeK;

	// Configure font
	_font = "ttf-dejavu/DejaVuSans.ttf";
	if (config["font"]) _font = config["font"].as<std::string>();
	_fontSize = 8;
	if (config["fontsize"]) _fontSize = config["fontsize"].as<int>();

	// Configure freq range, default value to full range of positive and negative
	_fMin = -N / 2;
	_fMax = N / 2;
	if (config["freqmin"]) {
		long int f = config["freqmin"].as<long int>();

		if ((f >= -_sampleRate / 2) && (f <= _sampleRate / 2)) {
			_fMin = (int)((f * N) / _sampleRate);
		} else if ((f - _baseFreq >= -_sampleRate / 2) && (f - _baseFreq <= _sampleRate / 2)) {
			_fMin = (int)(((f - _baseFreq) * N) / _sampleRate);
		} else {
			throw std::runtime_error("QGImage::configure: freqmin out of range");
		}
	}
	if (config["freqmax"]) {
		long int f = config["freqmax"].as<long int>();

		if ((f >= -_sampleRate / 2) && (f <= _sampleRate / 2)) {
			_fMax = (int)((f * N) / _sampleRate);
		} else if ((f - _baseFreq >= -_sampleRate / 2) && (f - _baseFreq <= _sampleRate / 2)) {
			_fMax = (int)(((f - _baseFreq) * N) / _sampleRate);
		} else {
			throw std::runtime_error("QGImage::configure: freqmax out of range");
		}
	}
	if (_fMin >= _fMax) std::runtime_error("QGImage::configure: freqmin must be lower than freqmax");
	_fDelta = _fMax - _fMin;

	// Configure db range
	_dBmin = -30;
	_dBmax = 0;
	if (config["dBmin"]) _dBmin = config["dBmin"].as<int>();
	if (config["dBmax"]) _dBmax = config["dBmax"].as<int>();
	_dBdelta = _dBmax - _dBmin;

	// Optionally don't align frames
	_alignFrame = true;
	if (config["noalign"]) {
		std::string s = config["noalign"].as<std::string>();
		if ((s.compare("true") == 0) || (s.compare("yes") == 0)) _alignFrame = false;
		else if ((s.compare("false") == 0) || (s.compare("no") == 0)) _alignFrame = true;
		else throw std::runtime_error("QGImage::configure: illegal value for noalign");
	}

	// Configure start time
	_syncFrames = true;
	if (config["started"]) {
		using namespace std::chrono;

		std::tm tm = {};
		std::stringstream ss(config["started"].as<std::string>());

		// Don't attempt to resync on frame start if start time given. Usually start time is only used when processing offline data.
		_syncFrames = false;

		//ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ"); // std::get_time() not supported on Travis' Ubuntu 14.04 build system
		std::string c; // to hold separators
		ss >> std::setw(4) >> tm.tm_year >> std::setw(1) >> c >> std::setw(2) >> tm.tm_mon >> std::setw(1) >> c >> std::setw(2) >> tm.tm_mday >> std::setw(1) >> c
			>> std::setw(2) >> tm.tm_hour >> std::setw(1) >> c >> std::setw(2) >> tm.tm_min >> std::setw(1) >> c >> std::setw(2) >> tm.tm_sec >> std::setw(1) >> c;
		tm.tm_year -= 1900; // Fix to years since 1900s
		tm.tm_mon -= 1; // Fix to 0-11 range

		// mktime takes time in local time. Should be taken in UTC, patched below
		_started = duration_cast<milliseconds>(system_clock::from_time_t(std::mktime(&tm)).time_since_epoch());

		// Calculate difference from UTC and local winter time to fix _started
		time_t t0 = time(nullptr);

		// Fix started time to be in UTC
		_started += seconds(mktime(localtime(&t0)) - mktime(gmtime(&t0)));
	}

	// Not yet configurable values
	_markerSize = ceil(1.2 * _fontSize);
	_borderSize = _markerSize / 2;

	_init();

	_computeFreqScale();
	_computeTimeScale();

	_drawDbScale(); // draw first as still overlaps with freq scales
	_drawFreqScale();

	startNewFrame(false);
}

void QGImage::startNewFrame(bool incrementTime) {
	int black = gdTrueColor(0, 0, 0);

	switch (_orientation) {
	case Orientation::Horizontal:
		gdImageFilledRectangle(_im,
			_borderSize + _freqLabelWidth + _markerSize,
			_borderSize + _titleHeight + _markerSize,
			_borderSize + _freqLabelWidth + _markerSize + _size - 1,
			_borderSize + _titleHeight + _markerSize + _fDelta - 1,
			black);
		gdImageFilledRectangle(_im,
			_borderSize + _freqLabelWidth + _markerSize + _size + _markerSize + _freqLabelWidth,
			_borderSize + _titleHeight + _markerSize,
			_borderSize + _freqLabelWidth + _markerSize + _size + _markerSize + _freqLabelWidth + _scopeSize - 1,
			_borderSize + _titleHeight + _markerSize + _fDelta - 1,
			black);
		break;

	case Orientation::Vertical:
		gdImageFilledRectangle(_im,
			_borderSize + _timeLabelWidth + _markerSize,
			_borderSize + _titleHeight + _freqLabelHeight + _markerSize,
			_borderSize + _timeLabelWidth + _markerSize + _fDelta - 1,
			_borderSize + _titleHeight + _freqLabelHeight + _markerSize + _size - 1,
			black);
		gdImageFilledRectangle(_im,
			_borderSize + _timeLabelWidth + _markerSize,
			_borderSize + _titleHeight + _freqLabelHeight + _markerSize + _size + _markerSize + _freqLabelHeight,
			_borderSize + _timeLabelWidth + _markerSize + _fDelta - 1,
			_borderSize + _titleHeight + _freqLabelHeight + _markerSize + _size + _markerSize + _freqLabelHeight + _scopeSize - 1,
			black);
		break;
	}

	using namespace std::chrono;

	bool backSync = false; // Syncing can bring the current line into the past regarding new current frame, better said the new frame was created to early
	_started += seconds(_secondsPerFrame);

	if (_syncFrames) {
		milliseconds s = _started;
		_started = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
		backSync = (_started < s) ? true : false;
		if (incrementTime) std::cout << (_started - s).count() << "ms adjustement on new frame" << std::endl; // TODO: incrementTime is missused as 'not first frame'. Should be renamed as initial use not needed anymore
	}

	if (_alignFrame) {
		_startedIntoFrame = _started % seconds(_secondsPerFrame); // Time into frame to align frames on correct boundary
		if (backSync) _startedIntoFrame -= seconds(_secondsPerFrame); // Negative value
		_started -= _startedIntoFrame;
		_currentLine = (_startedIntoFrame.count() * _timeK) / 1000;
	} else {
		_startedIntoFrame = milliseconds(0);
		_currentLine = 0;
	}

	_drawTimeScale();
}

QGImage::Status QGImage::addLine(const std::complex<float> *fft) {
	if (_currentLine < 0) {
		_currentLine++;
		return Status::Ok;
	}
	if (_currentLine >= _size) return Status::FrameReady;

	int whiteA = gdTrueColorAlpha(255, 255, 255, 125);

	// Draw a data line DC centered
	float last;

	for (int i = _fMin; i < _fMax; i++) {
		// TODO: evaluate to do this in fft class once, for multi-image support
		float v = 10 * log10(abs(fft[(i + N) % N]) / N); // Current value, DC centered

		switch (_orientation) {
		case Orientation::Horizontal:
			// TODO precalculate position of waterfall ans scope in canvas. All values in loop could be precalculated, just i, v or last added or substracted in loop
			gdImageSetPixel(_im,
				_borderSize + _freqLabelWidth + _markerSize + _currentLine,
				_borderSize + _titleHeight + _markerSize + _fDelta - 1 + _fMin - i, // -1 is to compensate the + _fDelta which goes one pixel after the waterfall zone
				_db2Color(v));
			if (i != _fMin)
				gdImageLine(_im,
					_borderSize + _freqLabelWidth + _markerSize + _size + _markerSize + _freqLabelWidth - last,
					_borderSize + _titleHeight + _markerSize + _fDelta - 1 + _fMin - i + 1, // -1 as before, + 1 to have last position
					_borderSize + _freqLabelWidth + _markerSize + _size + _markerSize + _freqLabelWidth - v,
					_borderSize + _titleHeight + _markerSize + _fDelta - 1 + _fMin - i,
					whiteA);
			break;

		case Orientation::Vertical:
			gdImageSetPixel(_im,
				_borderSize + _timeLabelWidth + _markerSize - _fMin + i,
				_borderSize + _titleHeight + _freqLabelHeight + _markerSize + _currentLine,
				_db2Color(v));
			if (i != _fMin)
				gdImageLine(_im,
					_borderSize + _timeLabelWidth + _markerSize - _fMin + i - 1, // -1 to have last position
					_borderSize + _titleHeight + _freqLabelHeight + _markerSize + _size + _markerSize + _freqLabelHeight - last,
					_borderSize + _timeLabelWidth + _markerSize - _fMin + i,
					_borderSize + _titleHeight + _freqLabelHeight + _markerSize + _size + _markerSize + _freqLabelHeight - v,
					whiteA);
			break;
		}

		last = v;
	}

	_currentLine++;

	if (_currentLine >= _size) return Status::FrameReady;

	return Status::Ok;
}

char *QGImage::getFrame(int *frameSize, std::string &frameName) {
	if (_imBuffer) gdFree(_imBuffer);

	// _imBuffer is usually constant, but frameSize changes
	// _imBuffer might change in case of realloc, so don't cache its value
	_imBuffer = (char *)gdImagePngPtr(_im, frameSize);

	time_t t = std::chrono::duration_cast<std::chrono::seconds>(_started).count();
	std::tm *tm = {};
	tm = std::gmtime(&t);
	char s[21];
	std::strftime(s, sizeof(s), "%FT%TZ", tm);
	frameName = std::string(s) + "_" + std::to_string(_baseFreq) + "Hz.png";

	return _imBuffer;
}

// Private members
void QGImage::_init() {
	int white = gdTrueColor(255, 255, 255);
	int black = gdTrueColor(0, 0, 0); // Not that useful, but once we want to set another background color the code is already here

	_scopeSize = 100;
	_titleHeight = 10;

	// Calculate max bounding boxes for labels, check font existence on first call
	int brect[8];

	std::stringstream ss;
	ss << QrssPiG_NAME << " v" << QrssPiG_VERSION_MAJOR << "." << QrssPiG_VERSION_MINOR << "." << QrssPiG_VERSION_PATCH;
	_qrsspigString = ss.str();
	char * err = gdImageStringFT(nullptr, brect, 0, const_cast<char *>(_font.c_str()), _fontSize, 0, 0, 0, const_cast<char *>(_qrsspigString.c_str()));
	if (err) throw std::runtime_error(err);
	_qrsspigLabelWidth = brect[2] - brect[0];
	_qrsspigLabelHeight = brect[1] - brect[7];
	int qrsspigLabelBase = brect[1];

	gdImageStringFT(nullptr, brect, 0, const_cast<char *>(_font.c_str()), _fontSize, 0, 0, 0, const_cast<char *>("000000000Hz"));
	_freqLabelWidth = brect[2] - brect[0];
	_freqLabelHeight = brect[1] - brect[7];

	gdImageStringFT(nullptr, brect, 0, const_cast<char *>(_font.c_str()), _fontSize, 0, 0, 0, const_cast<char *>("-100dB"));
	_dBLabelWidth = brect[2] - brect[0];
	_dBLabelHeight = brect[1] - brect[7];

	gdImageStringFT(nullptr, brect, 0, const_cast<char *>(_font.c_str()), _fontSize, 0, 0, 0, const_cast<char *>("00:00:00"));
	_timeLabelWidth = brect[2] - brect[0];
	_timeLabelHeight = brect[1] - brect[7];

	// Allocate canvas
	switch (_orientation) {
	case Orientation::Horizontal:
		_im = gdImageCreateTrueColor(
			_borderSize + _freqLabelWidth + _markerSize + _size + _markerSize + _freqLabelWidth + _scopeSize + _borderSize,
			_borderSize + _titleHeight + _markerSize + _fDelta + _markerSize + _timeLabelHeight + _borderSize);
		gdImageFilledRectangle(_im, 0, 0,
			_borderSize + _freqLabelWidth + _markerSize + _size + _markerSize + _freqLabelWidth + _scopeSize + _borderSize,
			_borderSize + _titleHeight + _markerSize + _fDelta + _markerSize + _timeLabelHeight + _borderSize - 1,
			black);
		gdImageStringFT(_im, brect, white, const_cast<char *>(_font.c_str()), _fontSize, 0,
			_borderSize + _freqLabelWidth + _markerSize + _size + _markerSize + _freqLabelWidth + _scopeSize - _qrsspigLabelWidth,
			_borderSize + _titleHeight + _markerSize + _fDelta + _markerSize + _timeLabelHeight - qrsspigLabelBase,
			const_cast<char *>(_qrsspigString.c_str()));
		break;

	case Orientation::Vertical:
		_im = gdImageCreateTrueColor(
			_borderSize + _timeLabelWidth + _markerSize + _fDelta + _borderSize,
			_borderSize + _titleHeight + _freqLabelHeight + _markerSize + _size + _markerSize + _freqLabelHeight + _scopeSize + _borderSize);
		gdImageFilledRectangle(_im, 0, 0,
			_borderSize + _timeLabelWidth + _markerSize + _fDelta + _borderSize,
			_borderSize + _titleHeight + _freqLabelHeight + _markerSize + _size + _markerSize + _freqLabelHeight + _scopeSize + _borderSize - 1,
			black);
		gdImageStringFT(_im, brect, white, const_cast<char *>(_font.c_str()), _fontSize, 0,
			_borderSize + _timeLabelWidth + _markerSize + _fDelta - _qrsspigLabelWidth,
			_borderSize + _titleHeight + _freqLabelHeight + _markerSize + _size + _markerSize + _freqLabelHeight + _scopeSize - qrsspigLabelBase,
			const_cast<char *>(_qrsspigString.c_str()));
		break;
	}

	// Allocate colormap, based on modified qrx colormap reduced to 256 colors
	_cd = 256;
	_c = new int[_cd];

	int ii = 0;
	for (int i = 0; i <= 255; i+= 4) _c[ii++] = gdImageColorAllocate(_im, 0, 0, i);
	for (int i = 0; i <= 255; i+= 4) _c[ii++] = gdImageColorAllocate(_im, 0, i, 255);
	for (int i = 0; i <= 255; i+= 4) _c[ii++] = gdImageColorAllocate(_im, i, 255, 255 - i);
	for (int i = 0; i <= 255; i+= 4) _c[ii++] = gdImageColorAllocate(_im, 255, 255 - i, 0);

	_currentLine = 0;
}

void QGImage::_free() {
	if (_c) delete [] _c;
	_c = nullptr;

	if (_imBuffer) gdFree(_imBuffer);
	_imBuffer = nullptr;

	if (_im) gdImageDestroy(_im);
	_im = nullptr;
}

void QGImage::_computeFreqScale() {
	// Calculate label spacing
	int maxLabelSize;
	if (_orientation == Orientation::Horizontal) {
		maxLabelSize = 3 * _freqLabelHeight;
	} else {
		maxLabelSize = (6 * _freqLabelWidth) / 5;
	}

	// Search finest label interval withoug overlap
	if (1 * _freqK > maxLabelSize) { _hertzPerFreqLabel = 1; _freqLabelDivs = 10; }
	else if (2 * _freqK > maxLabelSize) { _hertzPerFreqLabel = 2; _freqLabelDivs = 2; }
	else if (5 * _freqK > maxLabelSize) { _hertzPerFreqLabel = 5; _freqLabelDivs = 5; }
	else if (10 * _freqK > maxLabelSize) { _hertzPerFreqLabel = 10; _freqLabelDivs = 10; }
	else if (25 * _freqK > maxLabelSize) { _hertzPerFreqLabel = 25; _freqLabelDivs = 5; }
	else if (50 * _freqK > maxLabelSize) { _hertzPerFreqLabel = 50; _freqLabelDivs = 10; }
	else if (100 * _freqK > maxLabelSize) { _hertzPerFreqLabel = 100; _freqLabelDivs = 10; }
	else if (250 * _freqK > maxLabelSize) { _hertzPerFreqLabel = 250; _freqLabelDivs = 5; }
	else if (500 * _freqK > maxLabelSize) { _hertzPerFreqLabel = 500; _freqLabelDivs = 10; }
	else if (1000 * _freqK > maxLabelSize) { _hertzPerFreqLabel = 1000; _freqLabelDivs = 10; }
	else if (2500 * _freqK > maxLabelSize) { _hertzPerFreqLabel = 2500; _freqLabelDivs = 5; }
	else if (5000 * _freqK > maxLabelSize) { _hertzPerFreqLabel = 5000; _freqLabelDivs = 10; }
	else { _hertzPerFreqLabel = 10000; _freqLabelDivs = 10; }
}

void QGImage::_drawFreqScale() {
	int black = gdTrueColor(0, 0, 0);
	int white = gdTrueColor(255, 255, 255);

	// Clear zones and set topLeft corners
	if (_orientation == Orientation::Horizontal) {
		gdImageFilledRectangle(_im,
			_borderSize,
			_borderSize + _titleHeight,
			_borderSize + _freqLabelWidth - 1,
			_borderSize + _titleHeight + _markerSize + _fDelta + _markerSize - 1,
			black);
		gdImageFilledRectangle(_im,
			_borderSize + _freqLabelWidth,
			_borderSize + _titleHeight + _markerSize,
			_borderSize + _freqLabelWidth + _markerSize - 1,
			_borderSize + _titleHeight + _markerSize + _fDelta - 1,
			black);
		gdImageFilledRectangle(_im,
			_borderSize + _freqLabelWidth + _markerSize + _size,
			_borderSize + _titleHeight + _markerSize,
			_borderSize + _freqLabelWidth + _markerSize + _size + _markerSize - 1,
			_borderSize + _titleHeight + _markerSize + _fDelta - 1,
			black);
		gdImageFilledRectangle(_im,
			_borderSize + _freqLabelWidth + _markerSize + _size + _markerSize,
			_borderSize + _titleHeight,
			_borderSize + _freqLabelWidth + _markerSize + _size + _markerSize + _freqLabelWidth - 1,
			_borderSize + _titleHeight + _markerSize + _fDelta + _markerSize - 1,
			black);
	} else {
		gdImageFilledRectangle(_im,
			_borderSize + _timeLabelWidth + _markerSize - _freqLabelWidth/2,
			_borderSize + _titleHeight,
			_borderSize + _timeLabelWidth + _markerSize + _fDelta + _freqLabelWidth/2 - 1,
			_borderSize + _titleHeight + _freqLabelHeight - 1,
			black);
		gdImageFilledRectangle(_im,
			_borderSize + _timeLabelWidth + _markerSize,
			_borderSize + _titleHeight + _freqLabelHeight,
			_borderSize + _timeLabelWidth + _markerSize + _fDelta - 1,
			_borderSize + _titleHeight + _freqLabelHeight + _markerSize - 1,
			black);
		gdImageFilledRectangle(_im,
			_borderSize + _timeLabelWidth + _markerSize,
			_borderSize + _titleHeight + _freqLabelHeight + _markerSize + _size,
			_borderSize + _timeLabelWidth + _markerSize + _fDelta - 1,
			_borderSize + _titleHeight + _freqLabelHeight + _markerSize + _size + _markerSize - 1,
			black);
		gdImageFilledRectangle(_im,
			_borderSize + _timeLabelWidth + _markerSize - _freqLabelWidth/2,
			_borderSize + _titleHeight + _freqLabelHeight + _markerSize + _size + _markerSize,
			_borderSize + _timeLabelWidth + _markerSize + _fDelta + _freqLabelWidth/2 - 1,
			_borderSize + _titleHeight + _freqLabelHeight + _markerSize + _size + _markerSize + _freqLabelHeight - 1,
			black);
	}

	int f0;

	// Freq labels with long ticks
	f0 = ((int)(_fMin / _freqK - 1) / _hertzPerFreqLabel) * _hertzPerFreqLabel;

	for (int f = f0; f <= _fMax / _freqK + 1; f += _hertzPerFreqLabel) { // + 1 to include high freq label
		int p = f * _freqK; // Position of label

		std::stringstream s;
		s << (_baseFreq + f) << "Hz";

		// Calculate text's bounding box
		int brect[8];
		gdImageStringFT(nullptr, brect, 0, (char *)_font.c_str(), _fontSize, 0, 0, 0, (char *)s.str().c_str());

		// Cache key data as they will be overriden when rendering first string
		int x = brect[0], y = brect[1], w = brect[2] - brect[0], h = brect[1] - brect[7];

		if (_orientation == Orientation::Horizontal) {
			gdImageStringFT(_im, brect, white, const_cast<char *>(_font.c_str()), _fontSize, 0,
			 	_borderSize + _freqLabelWidth - w,
				_borderSize + _titleHeight + _markerSize + _fDelta - 1 + _fMin - p - y + .5 * h, // -1 to compensate + fDelta which goes one too far
			 	const_cast<char *>(s.str().c_str()));
			gdImageLine(_im,
				_borderSize + _freqLabelWidth,
				_borderSize + _titleHeight + _markerSize + _fDelta - 1 + _fMin - p, // -1 to compensate + fDelta which goes one too far
				_borderSize + _freqLabelWidth + _markerSize - 1,
				_borderSize + _titleHeight + _markerSize + _fDelta - 1 + _fMin - p, // -1 to compensate + fDelta which goes one too far
				white);
			gdImageLine(_im,
				_borderSize + _freqLabelWidth + _markerSize + _size,
				_borderSize + _titleHeight + _markerSize + _fDelta - 1 + _fMin - p, // -1 to compensate + fDelta which goes one too far
				_borderSize + _freqLabelWidth + _markerSize + _size + _markerSize - 1,
				_borderSize + _titleHeight + _markerSize + _fDelta - 1 + _fMin - p, // -1 to compensate + fDelta which goes one too far
				white);
			gdImageStringFT(_im, brect, white, const_cast<char *>(_font.c_str()), _fontSize, 0,
				_borderSize + _freqLabelWidth + _markerSize + _size + _markerSize,
				_borderSize + _titleHeight + _markerSize + _fDelta - 1 + _fMin - p - y + .5 * h, // -1 to compensate + fDelta which goes one too far
				const_cast<char *>(s.str().c_str()));
		} else {
			gdImageStringFT(_im, brect, white, const_cast<char *>(_font.c_str()), _fontSize, 0,
				_borderSize + _timeLabelWidth + _markerSize - _fMin + p - w/2,
				_borderSize + _titleHeight + _freqLabelHeight - y,
				const_cast<char *>(s.str().c_str()));
			gdImageLine(_im,
				_borderSize + _timeLabelWidth + _markerSize - _fMin + p,
				_borderSize + _titleHeight + _freqLabelHeight,
				_borderSize + _timeLabelWidth + _markerSize - _fMin + p,
				_borderSize + _titleHeight + _freqLabelHeight + _markerSize - 1,
				white);
			gdImageLine(_im,
				_borderSize + _timeLabelWidth + _markerSize - _fMin + p,
				_borderSize + _titleHeight + _freqLabelHeight + _markerSize + _size,
				_borderSize + _timeLabelWidth + _markerSize - _fMin + p,
				_borderSize + _titleHeight + _freqLabelHeight + _markerSize + _size + _markerSize - 1,
				white);
			gdImageStringFT(_im, brect, white, const_cast<char *>(_font.c_str()), _fontSize, 0,
				_borderSize + _timeLabelWidth + _markerSize - _fMin + p - w/2,
				_borderSize + _titleHeight + _freqLabelHeight + _markerSize + _size + _markerSize + _freqLabelHeight - y,
				const_cast<char *>(s.str().c_str()));
		}
	}

	// Small tick markers
	f0 = ((int)(_fMin / _freqK - 1) / (_hertzPerFreqLabel / _freqLabelDivs)) * (_hertzPerFreqLabel / _freqLabelDivs);

	for (float f = f0; f <= _fMax / _freqK + 1; f += (float)_hertzPerFreqLabel / _freqLabelDivs) {
		int p = f * _freqK; // Position of label

		if (_orientation == Orientation::Horizontal) {
			gdImageLine(_im,
				_borderSize + _freqLabelWidth + _markerSize/2,
				_borderSize + _titleHeight + _markerSize + _fDelta - 1 + _fMin - p, // -1 to compensate + fDelta which goes one too far
				_borderSize + _freqLabelWidth + _markerSize - 1,
				_borderSize + _titleHeight + _markerSize + _fDelta - 1 + _fMin - p, // -1 to compensate + fDelta which goes one too far
				white);
			gdImageLine(_im,
				_borderSize + _freqLabelWidth + _markerSize + _size,
				_borderSize + _titleHeight + _markerSize + _fDelta - 1 + _fMin - p, // -1 to compensate + fDelta which goes one too far
				_borderSize + _freqLabelWidth + _markerSize + _size + _markerSize/2 - 1,
				_borderSize + _titleHeight + _markerSize + _fDelta - 1 + _fMin - p, // -1 to compensate + fDelta which goes one too far
				white);
		} else {
			gdImageLine(_im,
				_borderSize + _timeLabelWidth + _markerSize - _fMin + p,
				_borderSize + _titleHeight + _freqLabelHeight + _markerSize/2,
				_borderSize + _timeLabelWidth + _markerSize - _fMin + p,
				_borderSize + _titleHeight + _freqLabelHeight + _markerSize - 1,
				white);
			gdImageLine(_im,
				_borderSize + _timeLabelWidth + _markerSize - _fMin + p,
				_borderSize + _titleHeight + _freqLabelHeight + _markerSize + _size,
				_borderSize + _timeLabelWidth + _markerSize - _fMin + p,
				_borderSize + _titleHeight + _freqLabelHeight + _markerSize + _size + _markerSize/2 - 1,
				white);
		}
	}
}

void QGImage::_drawDbScale() {
	int tickStep, labelStep;
	int black = gdTrueColor(0, 0, 0);
	int white = gdTrueColor(255, 255, 255);

	// Clear zone and set topLeft corner
	if (_orientation == Orientation::Horizontal) {
		tickStep = 10;
		labelStep = tickStep * 3;
		gdImageFilledRectangle(_im,
			_borderSize + _freqLabelWidth + _markerSize + _size + _markerSize + _freqLabelWidth,
			_borderSize + _titleHeight + _markerSize + _fDelta,
			_borderSize + _freqLabelWidth + _markerSize + _size + _markerSize + _freqLabelWidth + _scopeSize - 1,
			_borderSize + _titleHeight + _markerSize + _fDelta + _markerSize - 1,
			black);
		gdImageFilledRectangle(_im,
			_borderSize + _freqLabelWidth + _markerSize + _size + _markerSize + _freqLabelWidth - _dBLabelWidth/2,
			_borderSize + _titleHeight + _markerSize + _fDelta + _markerSize,
			_borderSize + _freqLabelWidth + _markerSize + _size + _markerSize + _freqLabelWidth + _scopeSize + _dBLabelWidth/2 - 1,
			_borderSize + _titleHeight + _markerSize + _fDelta + _markerSize + _dBLabelHeight - 1,
			black);
	} else {
		tickStep = 10;
		labelStep = tickStep;
		gdImageFilledRectangle(_im,
			_borderSize,
			_borderSize + _titleHeight + _freqLabelHeight + _markerSize + _size + _markerSize + _freqLabelHeight - _dBLabelHeight/2,
			_borderSize + _timeLabelWidth - 1, // timeLabel is usually wider than dBLabel.. should take larger of them
			_borderSize + _titleHeight + _freqLabelHeight + _markerSize + _size + _markerSize + _freqLabelHeight + _scopeSize + _dBLabelHeight/2 - 1,
			black);
		gdImageFilledRectangle(_im,
			_borderSize + _timeLabelWidth,
			_borderSize + _titleHeight + _freqLabelHeight + _markerSize + _size + _markerSize + _freqLabelHeight,
			_borderSize + _timeLabelWidth + _markerSize - 1, // timeLabel is usually wider.. should take larger of them
			_borderSize + _titleHeight + _freqLabelHeight + _markerSize + _size + _markerSize + _freqLabelHeight + _scopeSize - 1,
			black);
	}

	// Render graph scale (hard coded to full range -100dB-0dB over 100 pixels)

	// dB labels with long ticks
	for (int i = 0; i > -100; i -= labelStep) {
		std::stringstream text;
		text << i << "dB";

		// Calculate text's bounding box
		int brect[8];
		gdImageStringFT(nullptr, brect, white, (char *)_font.c_str(), _fontSize, 0, 0, 0, (char *)text.str().c_str());

		if (_orientation == Orientation::Horizontal) {
			gdImageLine(_im,
				_borderSize + _freqLabelWidth + _markerSize + _size + _markerSize + _freqLabelWidth - i,
				_borderSize + _titleHeight + _markerSize + _fDelta,
				_borderSize + _freqLabelWidth + _markerSize + _size + _markerSize + _freqLabelWidth - i,
				_borderSize + _titleHeight + _markerSize + _fDelta + _markerSize - 1,
				white);
			gdImageStringFT(_im, brect, white, const_cast<char *>(_font.c_str()), _fontSize, 0,
				_borderSize + _freqLabelWidth + _markerSize + _size + _markerSize + _freqLabelWidth - i - .5 * (brect[2] - brect[0]),
				_borderSize + _titleHeight + _markerSize + _fDelta + _markerSize - brect[7],
				const_cast<char *>(text.str().c_str()));
		} else {
			gdImageStringFT(_im, brect, white, const_cast<char *>(_font.c_str()), _fontSize, 0,
				_borderSize + _timeLabelWidth - (brect[2] - brect[0]),
				_borderSize + _titleHeight + _freqLabelHeight + _markerSize + _size + _markerSize + _freqLabelHeight - i + (brect[1] - brect[7])/2 - brect[1],
				const_cast<char *>(text.str().c_str()));
			gdImageLine(_im,
				_borderSize + _timeLabelWidth, // timeLabel is usually wider than dBLabel.. should take larger of them
				_borderSize + _titleHeight + _freqLabelHeight + _markerSize + _size + _markerSize + _freqLabelHeight - i,
				_borderSize + _timeLabelWidth + _markerSize,
				_borderSize + _titleHeight + _freqLabelHeight + _markerSize + _size + _markerSize + _freqLabelHeight - i,
				white);
		}
	}

	// Small tick markers
	for (int i = 0; i > -100; i -= tickStep) {
		if (_orientation == Orientation::Horizontal)
			gdImageLine(_im,
				_borderSize + _freqLabelWidth + _markerSize + _size + _markerSize + _freqLabelWidth - i,
				_borderSize + _titleHeight + _markerSize + _fDelta,
				_borderSize + _freqLabelWidth + _markerSize + _size + _markerSize + _freqLabelWidth - i,
				_borderSize + _titleHeight + _markerSize + _fDelta + _markerSize/2 - 1,
				white);
		else
			gdImageLine(_im,
				_borderSize + _timeLabelWidth + _markerSize/2, // timeLabel is usually wider than dBLabel.. should take larger of them
				_borderSize + _titleHeight + _freqLabelHeight + _markerSize + _size + _markerSize + _freqLabelHeight - i,
				_borderSize + _timeLabelWidth + _markerSize,
				_borderSize + _titleHeight + _freqLabelHeight + _markerSize + _size + _markerSize + _freqLabelHeight - i,
				white);
	}

	// Indicator bar for dBmin-max range
	if (_orientation == Orientation::Horizontal)
		gdImageLine(_im,
			_borderSize + _freqLabelWidth + _markerSize + _size + _markerSize + _freqLabelWidth - _dBmin,
			_borderSize + _titleHeight + _markerSize + _fDelta + _markerSize/4,
			_borderSize + _freqLabelWidth + _markerSize + _size + _markerSize + _freqLabelWidth - _dBmax,
			_borderSize + _titleHeight + _markerSize + _fDelta + _markerSize/4,
			white);
	else
		gdImageLine(_im,
			_borderSize + _timeLabelWidth + _markerSize - _markerSize/4 - 1,
			_borderSize + _titleHeight + _freqLabelHeight + _markerSize + _size + _markerSize + _freqLabelHeight - _dBmin,
			_borderSize + _timeLabelWidth + _markerSize - _markerSize/4 - 1,
			_borderSize + _titleHeight + _freqLabelHeight + _markerSize + _size + _markerSize + _freqLabelHeight - _dBmax,
			white);

	// Color bar
	for (float i = -100.; i <= 0.; i++) {
		int c = _db2Color(i);

		if (_orientation == Orientation::Horizontal)
			gdImageLine(_im,
				_borderSize + _freqLabelWidth + _markerSize + _size + _markerSize + _freqLabelWidth - i,
				_borderSize + _titleHeight + _markerSize + _fDelta,
				_borderSize + _freqLabelWidth + _markerSize + _size + _markerSize + _freqLabelWidth - i,
				_borderSize + _titleHeight + _markerSize + _fDelta + _markerSize/4 - 1,
				c);
		else
			gdImageLine(_im,
				_borderSize + _timeLabelWidth + _markerSize - _markerSize/4,
				_borderSize + _titleHeight + _freqLabelHeight + _markerSize + _size + _markerSize + _freqLabelHeight - i,
				_borderSize + _timeLabelWidth + _markerSize - 1,
				_borderSize + _titleHeight + _freqLabelHeight + _markerSize + _size + _markerSize + _freqLabelHeight - i,
				c);
	}
}

void QGImage::_computeTimeScale() {
	// Calculate label spacing
	int maxLabelSize;
	if (_orientation == Orientation::Horizontal) {
		maxLabelSize = (6 * _timeLabelWidth) / 5;
	} else {
		maxLabelSize = 3 * _timeLabelHeight;
	}

	// Search finest label interval without overlap
	if (1 * _timeK > maxLabelSize) { _secondsPerTimeLabel = 1; _timeLabelDivs = 10; }
	else if (2 * _timeK > maxLabelSize) { _secondsPerTimeLabel = 2; _timeLabelDivs = 2; }
	else if (5 * _timeK > maxLabelSize) { _secondsPerTimeLabel = 5; _timeLabelDivs = 5; }
	else if (10 * _timeK > maxLabelSize) { _secondsPerTimeLabel = 10; _timeLabelDivs = 10; }
	else if (15 * _timeK > maxLabelSize) { _secondsPerTimeLabel = 15; _timeLabelDivs = 3; }
	else if (30 * _timeK > maxLabelSize) { _secondsPerTimeLabel = 30; _timeLabelDivs = 3; }
	else if (60 * _timeK > maxLabelSize) { _secondsPerTimeLabel = 60; _timeLabelDivs = 6; }
	else if (120 * _timeK > maxLabelSize) { _secondsPerTimeLabel = 120; _timeLabelDivs = 2; }
	else if (300 * _timeK > maxLabelSize) { _secondsPerTimeLabel = 300; _timeLabelDivs = 5; }
	else if (600 * _timeK > maxLabelSize) { _secondsPerTimeLabel = 600; _timeLabelDivs = 10; }
	else if (900 * _timeK > maxLabelSize) { _secondsPerTimeLabel = 900; _timeLabelDivs = 3; }
	else if (1800 * _timeK > maxLabelSize) { _secondsPerTimeLabel = 1800; _timeLabelDivs = 3; }
	else { _secondsPerTimeLabel = 3600; _timeLabelDivs = 6; }
}

void QGImage::_drawTimeScale() {
	int black = gdTrueColor(0, 0, 0);
	int white = gdTrueColor(255, 255, 255);

	// Clear zone and set topLeft corner
	if (_orientation == Orientation::Horizontal) {
		gdImageFilledRectangle(_im,
			_borderSize + _freqLabelWidth + _markerSize,
			_borderSize + _titleHeight + _markerSize + _fDelta,
			_borderSize + _freqLabelWidth + _markerSize + _size - 1,
			_borderSize + _titleHeight + _markerSize + _fDelta + _markerSize - 1,
			black);
		gdImageFilledRectangle(_im,
			_borderSize + _freqLabelWidth - _timeLabelWidth/2,
			_borderSize + _titleHeight + _markerSize + _fDelta + _markerSize,
			_borderSize + _freqLabelWidth + _markerSize + _size + _markerSize + _timeLabelWidth/2 - 1,
			_borderSize + _titleHeight + _markerSize + _fDelta + _markerSize + _timeLabelHeight - 1,
			black);
	} else {
		gdImageFilledRectangle(_im,
			_borderSize,
			_borderSize + _titleHeight + _freqLabelHeight,
			_borderSize + _timeLabelWidth - 1,
			_borderSize + _titleHeight + _freqLabelHeight + _markerSize + _size + _markerSize - 1,
			black);
		gdImageFilledRectangle(_im,
			_borderSize + _timeLabelWidth,
			_borderSize + _titleHeight + _freqLabelHeight + _markerSize,
			_borderSize + _timeLabelWidth + _markerSize - 1,
			_borderSize + _titleHeight + _freqLabelHeight + _markerSize + _size - 1,
			black);
	}

	int t0 = 0;

	// Time labels with long ticks

	// Realign labels on non-aligned frames
	if (_alignFrame == false) {
		t0 = _secondsPerTimeLabel - _startedIntoFrame.count() / 1000;
		while (t0 < 0) t0 += _secondsPerTimeLabel;
	}

	for (int t = t0; t < _secondsPerFrame; t += _secondsPerTimeLabel) {
		int l = t * _timeK; // Line number of label

		std::chrono::milliseconds ms = _started + std::chrono::seconds(t);
		int hh = std::chrono::duration_cast<std::chrono::hours>(ms).count() % 24;
		int mm = std::chrono::duration_cast<std::chrono::minutes>(ms).count() % 60;
		int ss = std::chrono::duration_cast<std::chrono::seconds>(ms).count() % 60;
		std::stringstream s;
		s << std::setfill('0') << std::setw(2) << hh << ":" << std::setw(2) << mm << ":" << std::setw(2) << ss;

		// Calculatre text's bounding box
		int brect[8];
		gdImageStringFT(nullptr, brect, white, const_cast<char *>(_font.c_str()), _fontSize, 0, 0, 0, const_cast<char *>(s.str().c_str()));

		if (_orientation == Orientation::Horizontal) {
			gdImageLine(_im,
				_borderSize + _freqLabelWidth + _markerSize + l,
				_borderSize + _titleHeight + _markerSize + _fDelta,
				_borderSize + _freqLabelWidth + _markerSize + l,
				_borderSize + _titleHeight + _markerSize + _fDelta + _markerSize - 1,
				white);
			gdImageStringFT(_im, brect, white, const_cast<char *>(_font.c_str()), _fontSize, 0,
				_borderSize + _freqLabelWidth + _markerSize + l - .5 * (brect[2] - brect[0]),
				_borderSize + _titleHeight + _markerSize + _fDelta + _markerSize + brect[1] - brect[7],
				const_cast<char *>(s.str().c_str()));
		} else {
			gdImageStringFT(_im, brect, white, const_cast<char *>(_font.c_str()), _fontSize, 0,
				_borderSize + _timeLabelWidth - (brect[2] - brect[0]),
				_borderSize + _titleHeight + _freqLabelHeight + _markerSize + l - .5 * (brect[1] + brect[7]) - brect[1],
				const_cast<char *>(s.str().c_str()));
			gdImageLine(_im,
				_borderSize + _timeLabelWidth,
				_borderSize + _titleHeight + _freqLabelHeight + _markerSize + l,
				_borderSize + _timeLabelWidth + _markerSize - 1,
				_borderSize + _titleHeight + _freqLabelHeight + _markerSize + l,
				white);
		}
	}

	// Small tick markers

	// Realign ticks on non-aligned frames
	if (_alignFrame == false) {
		t0 = _secondsPerTimeLabel / _timeLabelDivs - _startedIntoFrame.count() / 1000;
		while (t0 < 0) t0 += _secondsPerTimeLabel / _timeLabelDivs;
	}

	for (float t = t0; t < _secondsPerFrame; t += (float)_secondsPerTimeLabel / _timeLabelDivs) {
		int l = t * _timeK; // Line number of label

		if (_orientation == Orientation::Horizontal)
			gdImageLine(_im,
				_borderSize + _freqLabelWidth + _markerSize + l,
				_borderSize + _titleHeight + _markerSize + _fDelta,
				_borderSize + _freqLabelWidth + _markerSize + l,
				_borderSize + _titleHeight + _markerSize + _fDelta + _markerSize/2 - 1,
				white);
		else
			gdImageLine(_im,
				_borderSize + _timeLabelWidth + _markerSize/2,
				_borderSize + _titleHeight + _freqLabelHeight + _markerSize + l,
				_borderSize + _timeLabelWidth + _markerSize - 1,
				_borderSize + _titleHeight + _freqLabelHeight + _markerSize + l,
				white);
	}
}

int QGImage::_db2Color(float v) {
	if (v < _dBmin) v = _dBmin;
	if (v > _dBmax) v = _dBmax;

	return _c[(int)trunc((_cd - 1) * (v - _dBmin) / _dBdelta)];
}
