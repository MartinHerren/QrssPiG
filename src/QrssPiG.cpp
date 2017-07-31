#include "QrssPiG.h"

#include <stdexcept>
#include <string>

#include "QGLocalUploader.h"
#include "QGSCPUploader.h"

QrssPiG::QrssPiG() :
	_format(Format::U8IQ),
	_sampleRate(2000),
	_baseFreq(0),
	_up(nullptr) {
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
		_up = new QGSCPUploader(sshHost, sshUser, dir, sshPort);
	} else {
		_up = new QGLocalUploader(dir);
	}

	_init();
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

	_pushImage();

	if (_hannW) delete [] _hannW;
	if (_fft) delete _fft;
	if (_in) fftw_free(_in);
	if (_im) delete _im;
	if (_up) delete _up;
}

void QrssPiG::run() {
	std::cin >> std::noskipws;

	switch (_format) {
		case Format::U8IQ: {
			unsigned char i, q;
			while (std::cin) {
				i = std::cin.get();
				q = std::cin.get();
				_addIQ(std::complex<double>((i - 128) / 128., (q - 128) / 128.));
			}
			break;
		}

		case Format::S8IQ: {
			signed char i, q;
			while (std::cin) {
				i = std::cin.get();
				q = std::cin.get();
				_addIQ(std::complex<double>(i / 128., q / 128.));
			}
			break;
		}

		case Format::U16IQ: {
			unsigned short int i, q;
			while (std::cin) {
				i = std::cin.get();
				i += std::cin.get() << 8;
				q = std::cin.get();
				q += std::cin.get() << 8;
				_addIQ(std::complex<double>((i - 32768) / 32768., (q - 32768) / 32768.));
			}
			break;
		}

		case Format::S16IQ: {
			signed short int i, q;
			while (std::cin) {
				i = std::cin.get();
				i += std::cin.get() << 8;
				q = std::cin.get();
				q += std::cin.get() << 8;
				_addIQ(std::complex<double>(i / 32768., q / 32768.));
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

	// TODO remove once multiple uploader supported
	if (_up) delete _up;

	if (type.compare("scp") == 0) {
		std::string host = "localhost";
		int port = 22; // TODO: use 0 to force uploader to take default port ?
		std::string user = "";
		std::string dir = "./";

		if (uploader["host"]) host = uploader["host"].as<std::string>();
		if (uploader["port"]) port = uploader["port"].as<int>();
		if (uploader["user"]) user = uploader["user"].as<std::string>();
		if (uploader["dir"]) dir = uploader["dir"].as<std::string>();

		std::cout << "SCP uploader" << std::endl;

		_up = new QGSCPUploader(host, user, dir, port);
	} else if (type.compare("local") == 0) {
		std::string dir = "./";

		if (uploader["dir"]) dir = uploader["dir"].as<std::string>();

		std::cout << "Local uploader" << std::endl;

		_up = new QGLocalUploader(dir);
	}
}

void QrssPiG::_init() {
	_fft = new QGFft(_N);

	_in = (std::complex<double>*)fftw_malloc(sizeof(std::complex<double>) * _N);
	_fftIn = _fft->getInputBuffer();
	_fftOut = _fft->getFftBuffer();

	_im = new QGImage(_sampleRate, _baseFreq, _N, _overlap);

	_hannW = new double[_N];

	for (int i = 0; i < _N/2; i++) {
		_hannW[i] = .5 * (1 - cos((2 * M_PI * i) / (_N / 2 - 1)));
	}

	_idx = 0;
	_frameIndex = 0;
}

void QrssPiG::_addIQ(std::complex<double> iq) {
	_in[_idx++] = iq;

	if (_idx >= _N) {
		_computeFft();
		for (auto i = 0; i < _overlap; i++) _in[i] = _in[_N - _overlap + i];
		_idx = _overlap;
	}
}

void QrssPiG::_computeFft() {
	for (int i = 0; i < _N; i++) _fftIn[i] = _in[i] * _hannW[i / 2];
	_fft->process();
	if (_im->addLine(_fftOut) == QGImage::Status::FrameReady) _pushImage();
}

void QrssPiG::_pushImage() {
	try {
		int frameSize;
		std::string frameName;
		char * frame;

		frame = _im->getFrame(&frameSize, frameName);
		if (_up) _up->push(frameName, frame, frameSize);
std::cout << "pushed " << frameName << std::endl;

		_im->startNewFrame();
	} catch (const std::exception &e) {
		std::cerr << "Error pushing file: " << e.what() << std::endl;
	}

	_frameIndex++;
}
