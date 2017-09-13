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
	_markerSize = ceil(1.2 * _fontSize);

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
	using namespace std::chrono;
	_started = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
	if (config["started"]) {
		std::tm tm = {};
		std::stringstream ss(config["started"].as<std::string>());

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

	_startedIntoFrame = _started % seconds(_secondsPerFrame); // Time into first frame to align frames on correct boundary

	if (_alignFrame) {
		_started -= _startedIntoFrame;
	}

	_init();

	_computeFreqScale();
	_drawFreqScale();
	_drawDbScale();
	_computeTimeScale();

	startNewFrame(false);

	if (_alignFrame) {
		// Fix first _currentLine according to intoFrame, must be done after startNewFrame as startNewFrame sets it to zero
		_currentLine = (_startedIntoFrame.count() * _timeK) / 1000;
	}
}

void QGImage::startNewFrame(bool incrementTime) {
	switch (_orientation) {
	case Orientation::Horizontal:
		gdImageFilledRectangle(_im, _freqLabelWidth + _markerSize, _fDelta, _freqLabelWidth + _markerSize + _size + 100, 0, gdTrueColor(0, 0, 0));
		break;

	case Orientation::Vertical:
		gdImageFilledRectangle(_im, _dBLabelWidth + _markerSize, _freqLabelHeight + _markerSize + _size + 100, _dBLabelWidth + _markerSize + _fDelta, _freqLabelHeight + _markerSize, gdTrueColor(0, 0, 0));
		break;
	}

	if (incrementTime) _started += std::chrono::seconds(_secondsPerFrame);
	_drawTimeScale();

	_currentLine = 0;
}

QGImage::Status QGImage::addLine(const std::complex<float> *fft) {
	if (_currentLine >= _size) return Status::FrameReady;

	int whiteA = gdTrueColorAlpha(255, 255, 255, 125);

	// Draw a data line DC centered
	float last;

	for (int i = _fMin; i < _fMax; i++) {
		// TODO: evaluate to do this in fft class once, for multi-image support
		float v = 10 * log10(abs(fft[(i + N) % N]) / N); // Current value, DC centered

		switch (_orientation) {
		case Orientation::Horizontal:
			gdImageSetPixel(_im, _freqLabelWidth + _markerSize + _currentLine, _fDelta + _fMin - i, _db2Color(v));
			if (i != _fMin) gdImageLine(_im, _freqLabelWidth + _markerSize + _size - last, _fDelta + _fMin - i + 1, _freqLabelWidth + _markerSize + _size - v, _fDelta + _fMin - i, whiteA);
			break;

		case Orientation::Vertical:
			gdImageSetPixel(_im, _dBLabelWidth + _markerSize - _fMin + i, _freqLabelHeight + _markerSize + _currentLine, _db2Color(v));
			if (i != _fMin) gdImageLine(_im, _dBLabelWidth + _markerSize - _fMin + i - 1, _freqLabelHeight + _markerSize + _size - last, _dBLabelWidth + _markerSize - _fMin + i, _freqLabelHeight + _markerSize + _size - v, whiteA);
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
	// Calculate max bounding boxes for labels, check font existence on first call
	int brect[8];

	std::stringstream ss;
	ss << QrssPiG_NAME << " v" << QrssPiG_VERSION_MAJOR << "." << QrssPiG_VERSION_MINOR << "." << QrssPiG_VERSION_PATCH;
	_qrsspigString = ss.str();
	char * err = gdImageStringFT(nullptr, brect, 0, (char *)_font.c_str(), _fontSize, 0, 0, 0, const_cast<char *>(_qrsspigString.c_str()));
	if (err) throw std::runtime_error(err);
	_qrsspigLabelBase = brect[1];
	_qrsspigLabelWidth = brect[2] - brect[0];
	_qrsspigLabelHeight = brect[1] - brect[7];

	gdImageStringFT(nullptr, brect, 0, (char *)_font.c_str(), _fontSize, 0, 0, 0, const_cast<char *>("000000000Hz"));
	_freqLabelWidth = brect[2] - brect[0];
	_freqLabelHeight = brect[1] - brect[7];

	if (_orientation == Orientation::Horizontal) { // Ugly hack, using timelabel width for dB width until general layout code is done
		gdImageStringFT(nullptr, brect, 0, (char *)_font.c_str(), _fontSize, 0, 0, 0, const_cast<char *>("-100dB"));
	} else {
		gdImageStringFT(nullptr, brect, 0, (char *)_font.c_str(), _fontSize, 0, 0, 0, const_cast<char *>("00:00:00"));
	}
	_dBLabelWidth = brect[2] - brect[0];
	_dBLabelHeight = brect[1] - brect[7];

	// Allocate canvas
	switch (_orientation) {
	case Orientation::Horizontal:
		_im = gdImageCreateTrueColor(_freqLabelWidth + _markerSize + _size + 100 + _markerSize + _freqLabelWidth, _fDelta + _markerSize + _dBLabelHeight + _qrsspigLabelHeight);
		gdImageStringFT(_im, brect, white, const_cast<char *>(_font.c_str()), _fontSize, 0, _freqLabelWidth + _markerSize + _size + 100 + _markerSize + _freqLabelWidth - _qrsspigLabelWidth, _fDelta + _markerSize + _qrsspigLabelHeight + _qrsspigLabelHeight - _qrsspigLabelBase, const_cast<char *>(_qrsspigString.c_str()));
		break;

	case Orientation::Vertical:
		_im = gdImageCreateTrueColor(_dBLabelWidth + _markerSize + _fDelta, _freqLabelHeight + _markerSize + _size + 100 + _markerSize + _freqLabelHeight + _qrsspigLabelHeight);
		gdImageStringFT(_im, brect, white, const_cast<char *>(_font.c_str()), _fontSize, 0, _dBLabelWidth + _markerSize + _fDelta - _qrsspigLabelWidth, _freqLabelHeight + _markerSize + _size + 100 + _markerSize + _freqLabelHeight + _qrsspigLabelHeight - _qrsspigLabelBase, const_cast<char *>(_qrsspigString.c_str()));
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
	// Calculate text's max bounding box to determine label spacing
	int brect[8];
	gdImageStringFT(nullptr, brect, 0, const_cast<char *>(_font.c_str()), _fontSize, 0, 0, 0, const_cast<char *>("999999999MHz"));

	int maxLabelSize;
	if (_orientation == Orientation::Horizontal) {
		maxLabelSize = 3 * (brect[1] - brect[7]);
	} else {
		maxLabelSize = (6 * (brect[2] - brect[0])) / 5;
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
	int topLeftX, topLeftY, topLeftX2, topLeftY2;
	int black = gdTrueColor(0, 0, 0);
	int white = gdTrueColor(255, 255, 255);

	// Clear zones and set topLeft corners
	if (_orientation == Orientation::Horizontal) {
		topLeftX = 0;
		topLeftY = 0;
		topLeftX2 = _freqLabelWidth + _markerSize + _size + 100;
		topLeftY2 = 0;
		gdImageFilledRectangle(_im, topLeftX, topLeftY, topLeftX + _freqLabelWidth + _markerSize, topLeftY + _fDelta, black);
		gdImageFilledRectangle(_im, topLeftX2, topLeftY2, topLeftX2 + _markerSize + _freqLabelWidth, topLeftY2 + _fDelta, black);
	} else {
		topLeftX = _dBLabelWidth + _markerSize;
		topLeftY = 0;
		topLeftX2 = _dBLabelWidth + _markerSize;
		topLeftY2 = _freqLabelHeight + _markerSize + _size + 100;
		gdImageFilledRectangle(_im, topLeftX, topLeftY, topLeftX + _fDelta, topLeftY + _freqLabelHeight + _markerSize, black);
		gdImageFilledRectangle(_im, topLeftX2, topLeftY2, topLeftX2 + _fDelta, topLeftY2 + _freqLabelHeight + _markerSize, black);
	}

	int f0;

	// Freq labels with long ticks
	f0 = ((int)(_fMin / _freqK - 1) / _hertzPerFreqLabel) * _hertzPerFreqLabel + _hertzPerFreqLabel;

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
			gdImageStringFT(_im, brect, white, (char *)_font.c_str(), _fontSize, 0, topLeftX + _freqLabelWidth - w, topLeftY + _fDelta + _fMin - p - y + .5 * h, (char *)s.str().c_str());
			gdImageStringFT(_im, brect, white, (char *)_font.c_str(), _fontSize, 0, topLeftX2 + _markerSize, topLeftY2 + _fDelta + _fMin - p - y + .5 * h, (char *)s.str().c_str());
			gdImageLine(_im, topLeftX + _freqLabelWidth, topLeftY + _fDelta + _fMin - p, topLeftX + _freqLabelWidth + _markerSize, topLeftY + _fDelta + _fMin - p, white);
			gdImageLine(_im, topLeftX2, topLeftY2 + _fDelta + _fMin - p, topLeftX2 + _markerSize, topLeftY2 + _fDelta + _fMin - p, white);
		} else {
			gdImageStringFT(_im, brect, white, (char *)_font.c_str(), _fontSize, 0, topLeftX - _fMin + p - w/2, topLeftY + _freqLabelHeight, (char *)s.str().c_str());
			gdImageStringFT(_im, brect, white, (char *)_font.c_str(), _fontSize, 0, topLeftX2 - _fMin + p - w/2, topLeftY2 + _markerSize + _freqLabelHeight, (char *)s.str().c_str());
			gdImageLine(_im, topLeftX - _fMin + p, _freqLabelHeight, topLeftX - _fMin + p, _freqLabelHeight + _markerSize, white);
			gdImageLine(_im, topLeftX2 - _fMin + p, _freqLabelHeight + _markerSize + _size + 100, topLeftX2 - _fMin + p, _freqLabelHeight + _markerSize + _size + 100 + _markerSize, white);
		}
	}

	// Small tick markers
	f0 = ((int)(_fMin / _freqK - 1) / (_hertzPerFreqLabel / _freqLabelDivs)) * (_hertzPerFreqLabel / _freqLabelDivs) + (_hertzPerFreqLabel / _freqLabelDivs);

	for (int f = f0; f <= _fMax / _freqK + 1; f += _hertzPerFreqLabel / _freqLabelDivs) {
		int p = f * _freqK; // Position of label

		if (_orientation == Orientation::Horizontal) {
			gdImageLine(_im, topLeftX + _freqLabelWidth + _markerSize/2, topLeftY + _fDelta + _fMin - p, topLeftX + _freqLabelWidth + _markerSize, topLeftY + _fDelta + _fMin - p, white);
			gdImageLine(_im, topLeftX2, topLeftY2 + _fDelta + _fMin - p, topLeftX2 + _markerSize/2, topLeftY2 + _fDelta + _fMin - p, white);
		} else {
			gdImageLine(_im, topLeftX - _fMin + p, _freqLabelHeight + _markerSize/2, topLeftX - _fMin + p, _freqLabelHeight + _markerSize, white);
			gdImageLine(_im, topLeftX2 - _fMin + p, _freqLabelHeight + _markerSize + _size + 100, topLeftX2 - _fMin + p, _freqLabelHeight + _markerSize + _size + 100 + _markerSize/2, white);
		}
	}
}

void QGImage::_drawDbScale() {
	int topLeftX, topLeftY;
	int tickStep, labelStep;
	int black = gdTrueColor(0, 0, 0);
	int white = gdTrueColor(255, 255, 255);

	// Clear zone and set topLeft corner
	if (_orientation == Orientation::Horizontal) {
		topLeftX = _freqLabelWidth + _markerSize + _size;
		topLeftY = _fDelta;
		tickStep = 10;
		labelStep = tickStep * 3;
		gdImageFilledRectangle(_im, topLeftX, topLeftY, topLeftX + 100, topLeftY + _markerSize + _dBLabelHeight, black);
	} else {
		topLeftX = 0;
		topLeftY = _freqLabelHeight + _markerSize + _size;
		tickStep = 10;
		labelStep = tickStep;
		gdImageFilledRectangle(_im, topLeftX, topLeftY, topLeftX + _dBLabelWidth + _markerSize, topLeftY + 100, black);
	}

	// Render graph scale (hard coded to full range -100dB-0dB)

	// dB labels with long ticks
	for (int i = 0; i >= -100; i -= labelStep) {
		std::stringstream text;
		text << i << "dB";

		// Calculate text's bounding box
		int brect[8];
		gdImageStringFT(nullptr, brect, white, (char *)_font.c_str(), _fontSize, 0, 0, 0, (char *)text.str().c_str());

		// Fix position according to bounding box to be nicely aligned with tick-markers
		int x, y;

		if (_orientation == Orientation::Horizontal) {
			x = topLeftX - i - .5 * (brect[2] - brect[0]);
			y = topLeftY + _markerSize - brect[7];
			gdImageLine(_im, topLeftX - i, topLeftY + _markerSize, topLeftX - i, topLeftY + 7, white);
		} else {
			x = topLeftX +_dBLabelWidth - brect[2];
			y = topLeftY - i - .5 * (brect[1] + brect[7]);
			gdImageLine(_im, topLeftX + _dBLabelWidth, topLeftY - i, topLeftX + _dBLabelWidth + 3, topLeftY - i, white);
		}

		gdImageStringFT(_im, brect, white, (char *)_font.c_str(), _fontSize, 0, x, y, (char *)text.str().c_str());
	}

	// Small tick markers
	for (int i = 0; i >= -100; i -= tickStep) {
		if (_orientation == Orientation::Horizontal)
			gdImageLine(_im, topLeftX - i, topLeftY + 8, topLeftX - i, topLeftY + 7, white);
		else
			gdImageLine(_im, topLeftX + _dBLabelWidth + 2, topLeftY - i, topLeftX + _dBLabelWidth + 3, topLeftY - i, white);
	}

	// Indicator bar for dBmin-max range
	if (_orientation == Orientation::Horizontal)
		gdImageLine(_im, topLeftX - _dBmin, topLeftY + 6, topLeftX - _dBmax, topLeftY + 6, white);
	else
		gdImageLine(_im, topLeftX + _dBLabelWidth + 4, topLeftY - _dBmin, topLeftX +_dBLabelWidth + 4, topLeftY - _dBmax, white);

	// Color bar
	for (float i = -100.; i <= 0.; i++) {
		int c = _db2Color(i);

		if (_orientation == Orientation::Horizontal)
			gdImageLine(_im, topLeftX - i, topLeftY + 2, topLeftX - i, topLeftY + 5, c);
		else
			gdImageLine(_im, topLeftX + _dBLabelWidth + 5, topLeftY - i, topLeftX +_dBLabelWidth + 8, topLeftY - i, c);
	}
}

void QGImage::_computeTimeScale() {
	// Calculate text's max bounding box to determine label spacing
	int brect[8];
	gdImageStringFT(nullptr, brect, 0, const_cast<char *>(_font.c_str()), _fontSize, 0, 0, 0, const_cast<char *>("00:00:00"));

	int maxLabelSize;
	if (_orientation == Orientation::Horizontal) {
		maxLabelSize = (6 * (brect[2] - brect[0])) / 5;
	} else {
		maxLabelSize = 3 * (brect[1] - brect[7]);
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
	int topLeftX, topLeftY;
	int black = gdTrueColor(0, 0, 0);
	int white = gdTrueColor(255, 255, 255);

	// Clear zone and set topLeft corner
	if (_orientation == Orientation::Horizontal) {
		topLeftX = _freqLabelWidth + _markerSize;
		topLeftY = _fDelta;
		gdImageFilledRectangle(_im, 0, topLeftY + _markerSize, topLeftX, topLeftY + _markerSize + _dBLabelHeight, black);
		gdImageFilledRectangle(_im, topLeftX, topLeftY, topLeftX + _size, topLeftY + _markerSize + _dBLabelHeight, black);
	} else {
		topLeftX = 0;
		topLeftY = _freqLabelHeight + _markerSize;
		gdImageFilledRectangle(_im, topLeftX, topLeftY - _markerSize, topLeftX + _dBLabelWidth + _markerSize, topLeftY, black);
		gdImageFilledRectangle(_im, topLeftX, topLeftY, topLeftX + _dBLabelWidth + _markerSize, topLeftY + _size, black);
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
		gdImageStringFT(nullptr, brect, white, (char *)_font.c_str(), _fontSize, 0, 0, 0, const_cast<char *>(s.str().c_str()));

		// Fix position according to bounding box
		int x, y;

		if (_orientation == Orientation::Horizontal) {
			x = topLeftX + l - .5 * (brect[2] - brect[0]);
			y = topLeftY + _markerSize - brect[7];
			gdImageLine(_im, topLeftX + l, topLeftY, topLeftX + l, topLeftY + _markerSize, white);
		} else {
			x = topLeftX + _dBLabelWidth - brect[2];
			y = topLeftY + l - .5 * (brect[1] + brect[7]);
			gdImageLine(_im, topLeftX + _dBLabelWidth, topLeftY + l, topLeftX + _dBLabelWidth + _markerSize, topLeftY + l, white);
		}

		gdImageStringFT(_im, brect, white, (char *)_font.c_str(), _fontSize, 0, x, y, const_cast<char *>(s.str().c_str()));
	}

	// Small tick markers
	
	// Realign ticks on non-aligned frames
	if (_alignFrame == false) {
		t0 = _secondsPerTimeLabel / _timeLabelDivs - _startedIntoFrame.count() / 1000;
		while (t0 < 0) t0 += _secondsPerTimeLabel / _timeLabelDivs;
	}

	for (int t = t0; t < _secondsPerFrame; t += _secondsPerTimeLabel / _timeLabelDivs) {
		int l = t * _timeK; // Line number of label

		if (_orientation == Orientation::Horizontal)
			gdImageLine(_im, topLeftX + l, topLeftY, topLeftX + l, topLeftY + _markerSize/2, white);
		else
			gdImageLine(_im, topLeftX + _dBLabelWidth + _markerSize/2, topLeftY + l, topLeftX + _dBLabelWidth + _markerSize, topLeftY + l, white);
	}
}

int QGImage::_db2Color(float v) {
	if (v < _dBmin) v = _dBmin;
	if (v > _dBmax) v = _dBmax;

	return _c[(int)trunc((_cd - 1) * (v - _dBmin) / _dBdelta)];
}
