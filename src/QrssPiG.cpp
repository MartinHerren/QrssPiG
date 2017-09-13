#include "QrssPiG.h"

#include <stdexcept>
#include <string>

#include "QGLocalUploader.h"
#include "QGSCPUploader.h"

QrssPiG::QrssPiG() :
	_format(Format::U8IQ),
	_sampleRate(48000),
	_baseFreq(0),
	_chunkSize(32),
	_resampleRate(6000),
	_input(nullptr),
	_resampled(nullptr),
	_hannW(nullptr),
	_fftIn(nullptr),
	_fftOut(nullptr),
	_resampler(nullptr),
	_fft(nullptr),
	_im(nullptr) {
		_N = 2048;
		_overlap = (3 * _N) / 4;
}

QrssPiG::QrssPiG(const std::string &format, int sampleRate, int N, const std::string &dir, const std::string &sshHost, const std::string &sshUser, int sshPort) : QrssPiG() {
	if ((format.compare("u8iq") == 0) || (format.compare("rtlsdr") == 0)) _format = Format::U8IQ;
	else if ((format.compare("s8iq") == 0) || (format.compare("hackrf") == 0)) _format = Format::S8IQ;
	else if (format.compare("u16iq") == 0) _format = Format::U16IQ;
	else if (format.compare("s16iq") == 0) _format = Format::S16IQ;
	else throw std::runtime_error("Unsupported format");

	_N = N;
	_sampleRate = sampleRate;

	if (sshHost.length()) {
		_uploaders.push_back(new QGSCPUploader(sshHost, sshUser, dir, sshPort));
	} else {
		_uploaders.push_back(new QGLocalUploader(dir));
	}

	_init();

	_im->configure(YAML::Load("")); // Start with default config
}

QrssPiG::QrssPiG(const std::string &configFile) : QrssPiG() {
	YAML::Node config = YAML::LoadFile(configFile);

	if (config["input"]) {
		if (config["input"].Type() != YAML::NodeType::Map) throw std::runtime_error("YAML: input must be a map");

		YAML::Node input = config["input"];

		if (input["format"]) {
			std::string f = input["format"].as<std::string>();

			if ((f.compare("u8iq") == 0) || (f.compare("rtlsdr") == 0)) _format = Format::U8IQ;
			else if ((f.compare("s8iq") == 0) || (f.compare("hackrf") == 0)) _format = Format::S8IQ;
			else if (f.compare("u16iq") == 0) _format = Format::U16IQ;
			else if (f.compare("s16iq") == 0) _format = Format::S16IQ;
			else throw std::runtime_error("YAML: input format unrecognized");
		}

		if (input["samplerate"]) _sampleRate = input["samplerate"].as<int>();
		if (input["basefreq"]) _baseFreq = input["basefreq"].as<int>();
	}

	if (config["processing"]) {
		if (config["processing"].Type() != YAML::NodeType::Map) throw std::runtime_error("YAML: processing must be a map");

		YAML::Node processing = config["processing"];

		if (processing["chunksize"]) _chunkSize = processing["chunksize"].as<int>();
		if (processing["resamplerate"]) _resampleRate = processing["resamplerate"].as<int>();
		if (processing["fft"]) _N = processing["fft"].as<int>();
		if (processing["fftoverlap"]) {
			int o = processing["fftoverlap"].as<int>();
			if ((o < 0) || (o >= _N)) throw std::runtime_error("YAML: overlap value out of range [0..N[");
			_overlap = (o * _N) / (o + 1);
		}
	}

	// Must be done here, so that _im exists when configuring it
	_init();

	if (config["output"]) {
		if (config["output"].Type() != YAML::NodeType::Map) throw std::runtime_error("YAML: output must be a map");

		YAML::Node output = config["output"];

		_im->configure(output);
	} else {
		_im->configure(YAML::Load("")); // Start with default config
	}

	if (config["upload"]) {
		if ((config["upload"].Type() != YAML::NodeType::Map) &&
			(config["upload"].Type() != YAML::NodeType::Sequence)) {
			throw std::runtime_error("YAML: upload must be a map or a list");
		}

		if (config["upload"].Type() == YAML::NodeType::Map) {
			_addUploader(config["upload"]);
		} else if (config["upload"].Type() == YAML::NodeType::Sequence) {
			for (YAML::const_iterator it = config["upload"].begin(); it != config["upload"].end(); it++) {
				_addUploader(*it);
			}
		}
	}
}

QrssPiG::~QrssPiG() {
	// Draw residual data if any
	try {
		_im->addLine(_fftOut);
	} catch (const std::exception &e) {};

	// Push and wait on dtor
	_pushImage(true);

	if (_hannW) delete [] _hannW;
	if (_fft) delete _fft;
	if (_input) fftwf_free(_input);
	if (_resampled && (_resampled != _input)) fftwf_free(_resampled); // When no resampling is used both pointer point to the same buffer
	if (_im) delete _im;
	for (auto up: _uploaders) delete up;
	if (_resampler) delete _resampler;
}

void QrssPiG::run() {
	char b[8192];
	_running = true;
	std::cin >> std::noskipws;

	switch (_format) {
		case Format::U8IQ: {
			unsigned char i, q;
			while (std::cin && _running) {
				std::cin.read(b, 8192);
				for (int j = 0; j < 8192;) {
					i = b[j++];
					q = b[j++];
					_addIQ(std::complex<float>((i - 128) / 128., (q - 128) / 128.));
				}
			}
			break;
		}

		case Format::S8IQ: {
			signed char i, q;
			while (std::cin && _running) {
				std::cin.read(b, 8192);
				for (int j = 0; j < 8192;) {
					i = b[j++];
					q = b[j++];
					_addIQ(std::complex<float>(i / 128., q / 128.));
				}
			}
			break;
		}

		case Format::U16IQ: {
			unsigned short int i, q;
			while (std::cin && _running) {
				std::cin.read(b, 8192);
				for (int j = 0; j < 8192;) {
					i = b[j++];
					i += b[j++] << 8;
					q = b[j++];
					q += b[j++] << 8;
					_addIQ(std::complex<float>((i - 32768) / 32768., (q - 32768) / 32768.));
				}
			}
			break;
		}

		case Format::S16IQ: {
			signed short int i, q;
			while (std::cin && _running) {
				std::cin.read(b, 8192);
				for (int j = 0; j < 8192;) {
					i = b[j++];
					i += b[j++] << 8;
					q = b[j++];
					q += b[j++] << 8;
					_addIQ(std::complex<float>(i / 32768., q / 32768.));
				}
			}
			break;
		}
	}
}

//
// Private members
//
void QrssPiG::_addUploader(const YAML::Node &uploader) {
	if (!uploader["type"]) throw std::runtime_error("YAML: uploader must have a type");

	std::string type = uploader["type"].as<std::string>();

	if (type.compare("scp") == 0) {
		std::string host = "localhost";
		int port = 22; // TODO: use 0 to force uploader to take default port ?
		std::string user = "";
		std::string dir = "./";

		if (uploader["host"]) host = uploader["host"].as<std::string>();
		if (uploader["port"]) port = uploader["port"].as<int>();
		if (uploader["user"]) user = uploader["user"].as<std::string>();
		if (uploader["dir"]) dir = uploader["dir"].as<std::string>();

		_uploaders.push_back(new QGSCPUploader(host, user, dir, port));
	} else if (type.compare("local") == 0) {
		std::string dir = "./";

		if (uploader["dir"]) dir = uploader["dir"].as<std::string>();

		_uploaders.push_back(new QGLocalUploader(dir));
	}
}

void QrssPiG::_init() {
	if (_resampleRate && (_sampleRate != _resampleRate)) {
		_resampler = new QGDownSampler((float)_sampleRate/(float)_resampleRate, _chunkSize);
		_input = (std::complex<float>*)fftwf_malloc(sizeof(std::complex<float>) * _chunkSize);
		// Add provision to add a full resampled chunksize (+10% + 4 due to variable resampler output) when only one sample left before reaching N
		_resampled = (std::complex<float>*)fftwf_malloc(sizeof(std::complex<float>) * (_N + (1.1 * _chunkSize / _resampler->getRealRate() + 4) - 1));
		_inputIndexThreshold = _chunkSize; // Trig when input size reaches chunksize to start resampling
	} else {
		// Add provision to add a full chunksize when only one sample left before reaching N
		_input = (std::complex<float>*)fftwf_malloc(sizeof(std::complex<float>) * (_N + _chunkSize - 1));
		_resampled = _input;
		_inputIndexThreshold = _N; // Trig when input size reaches N to start fft
	}
	_inputIndex = 0;
	_resampledIndex = 0;

	_hannW = new float[_N];
	for (int i = 0; i < _N/2; i++) {
		_hannW[i] = .5 * (1 - cos((2 * M_PI * i) / (_N / 2 - 1)));
	}

	_fft = new QGFft(_N);
	_fftIn = _fft->getInputBuffer();
	_fftOut = _fft->getFftBuffer();

	_im = new QGImage(_sampleRate / (_resampler ? _resampler->getRealRate() : 1), _baseFreq, _N, _overlap);

	_frameIndex = 0;
}

void QrssPiG::_addIQ(std::complex<float> iq) {
	_input[_inputIndex++] = iq;

	if (_inputIndex >= _inputIndexThreshold) {
		if (_resampler) {
			// Adding 1 sample, we know _inputIndex == _inputIndexThreshold == _chunksSize, and not bigger
			_resampledIndex += _resampler->processChunk(_input, _resampled + _resampledIndex);
			_inputIndex = 0;
			if (_resampledIndex >= _N) {
				_computeFft();
				// TODO: Usually adds at most 1 sample, so we know _inputIndex == _inputIndexThreshold == _N, and not bigger
				for (auto i = 0; i < _overlap + _resampledIndex - _N; i++) _resampled[i] = _resampled[_N - _overlap + i];
				_resampledIndex = _overlap + _resampledIndex - _N;
			}
		} else {
			_computeFft();
			// Adding 1 sample, we know _inputIndex == _inputIndexThreshold == _N, and not bigger
			for (auto i = 0; i < _overlap; i++) _input[i] = _input[_N - _overlap + i];
			_inputIndex = _overlap;
		}
	}
}

void QrssPiG::_computeFft() {
	for (int i = 0; i < _N; i++) _fftIn[i] = _resampled[i] * _hannW[i / 2];
	_fft->process();
	if (_im->addLine(_fftOut) == QGImage::Status::FrameReady) _pushImage();
}

void QrssPiG::_pushImage(bool wait) {
	try {
		int frameSize;
		std::string frameName;
		char * frame;

		frame = _im->getFrame(&frameSize, frameName);

std::cout << "pushing " << frameName << std::endl;
		for (auto up: _uploaders) up->push(frameName, frame, frameSize, wait);

		_im->startNewFrame();
	} catch (const std::exception &e) {
		std::cerr << "Error pushing file: " << e.what() << std::endl;
	}

	_frameIndex++;
}
