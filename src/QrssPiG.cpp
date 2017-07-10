#include "QrssPiG.h"

#include <stdexcept>
#include <string>

#include "QGLocalUploader.h"
#include "QGSCPUploader.h"

QrssPiG::QrssPiG() :
	_N(2048),
	_unsignedIQ(true),
	_sampleRate(2000),
	_secondsPerFrame(600),
	_frameSize(1000),
	_up(nullptr) {
}

QrssPiG::QrssPiG(int N, bool unsignedIQ, int sampleRate, const std::string &dir, const std::string &sshHost, const std::string &sshUser, int sshPort) : QrssPiG() {
	_N = N;
	_unsignedIQ = unsignedIQ;
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
	double dBmin = -30., dBmax = 0.;

	if (config["N"]) _N = config["N"].as<int>();

	if (config["input"]) {
		if (config["input"].Type() != YAML::NodeType::Map) throw std::runtime_error("YAML: input must be a map");

		YAML::Node input = config["input"];

		if (input["format"]) {
			std::string f = input["format"].as<std::string>();

			if ((f.compare("rtlsdr") == 0) || (f.compare("unsigned") == 0)) {
				_unsignedIQ = true;
			} else if((f.compare("hackrf") == 0) || (f.compare("signed") == 0)) {
				_unsignedIQ = false;
			} else {
				throw std::runtime_error("YAML: input format unrecognized");
			}
		}

		if (input["samplerate"]) _sampleRate = input["samplerate"].as<int>();
	}

	if (config["output"]) {
		if (config["output"].Type() != YAML::NodeType::Map) throw std::runtime_error("YAML: output must be a map");

		YAML::Node output = config["output"];

		if (output["secondsperframe"]) _secondsPerFrame = output["secondsperframe"].as<int>();

		if (output["framesize"]) _frameSize = output["framesize"].as<int>();
		if (output["dBmin"]) dBmin = output["dBmin"].as<double>();
		if (output["dBmax"]) dBmax = output["dBmax"].as<double>();
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

	_init();

	_im->setScale(dBmin, dBmax);
}

QrssPiG::~QrssPiG() {
	// Draw residual data if any
	try {
		_im->drawLine(_fftOut, _lastLine);
	} catch (const std::exception &e) {};

	_pushImage();

	if (_hannW) delete [] _hannW;
	if (_fft) delete _fft;
	if (_in) fftw_free(_in);
	if (_im) delete _im;
	if (_up) delete _up;
}

void QrssPiG::addIQ(std::complex<double> iq) {
	int overlap = _N / 8; // 0.._N-1

	// Shift I/Q from [0,2] to [-1,1} interval for unsigned input
	if (_unsignedIQ) iq -= std::complex<double>(-1.,-1);

	_in[_idx++] = iq;
	_samples++;

	if (_idx >= _N) {
		_computeFft();
		for (auto i = 0; i < overlap; i++) _in[i] = _in[_N - overlap + i];
		_idx = overlap;
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
	using namespace std::chrono;
	_started = duration_cast<milliseconds>(system_clock::now().time_since_epoch());

	_fft = new QGFft(_N);

	_in = (std::complex<double>*)fftw_malloc(sizeof(std::complex<double>) * _N);
	_fftIn = _fft->getInputBuffer();
	_fftOut = _fft->getFftBuffer();

	_samplesPerLine = (double)(_sampleRate * _secondsPerFrame) / _frameSize;
	_linesPerSecond = (double)_frameSize / _secondsPerFrame;
	_lastLine = -1;
	_lastFrame = -1;

	_im = new QGImage(_frameSize, _sampleRate, _N);

	_hannW = new double[_N];

	for (int i = 0; i < _N/2; i++) {
		_hannW[i] = .5 * (1 - cos((2 * M_PI * i) / (_N / 2 - 1)));
	}

	_idx = 0;

	_timeInit();
}

void QrssPiG::_timeInit() {
	using namespace std::chrono;
	_started = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
	_samples = 0;
}

void QrssPiG::_computeFft() {
	using namespace std::chrono;

	long timeMs = _started.count() + 1000. * _samples / _sampleRate;
	long frameTimeMs = timeMs - (timeMs / (_secondsPerFrame * 1000)) * (_secondsPerFrame * 1000);
	long frameLine = int(frameTimeMs * _linesPerSecond / 1000) % _frameSize;

	_applyFilter();
	_fft->process();

	//if ((_lastLine > 0) && (_lastLine != frameLine)) {
		//_im->drawLine(_fftOut, frameLine);
		_im->drawLine(_fftOut, ++_lastLine);
		if (frameLine >= _frameSize - 1) _pushImage();
	//}

	//_lastLine = frameLine;
}

void QrssPiG::_applyFilter() {
	for (int i = 0; i < _N; i++) _fftIn[i] = _in[i] * _hannW[i / 2];
}

void QrssPiG::_pushImage() {
	std::string s("test");
	int p = 0;

	try {
		_im->save2Buffer();
		if (_up) _up->push(s + std::to_string(p) + ".png", _im->getBuffer(), _im->getBufferSize());
std::cout << "pushed" << std::endl;
	} catch (const std::exception &e) {
		std::cerr << "Error pushing file: " << e.what() << std::endl;
	}
	p++;
}
