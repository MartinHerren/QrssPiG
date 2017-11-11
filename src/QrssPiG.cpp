#include "QrssPiG.h"

#include <cstring>
#include <stdexcept>
#include <string>

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
using std::placeholders::_4;
using std::placeholders::_5;

QrssPiG::QrssPiG() :
	_chunkSize(32),
	_fftIn(nullptr),
	_fftOut(nullptr),
	_fft(nullptr) {
		_N = 2048;
		_overlap = (3 * _N) / 4;
}

QrssPiG::QrssPiG(const std::string &format, int sampleRate, int N, const std::string &dir, const std::string &sshHost, const std::string &sshUser, int sshPort) : QrssPiG() {
	_N = N;

	_inputDevice = QGInputDevice::CreateInputDevice(YAML::Load("{format: " + format + ", samplerate: " + std::to_string(sampleRate) + ", basefreq: 0}"));

	if (sshHost.length()) {
		_uploaders.push_back(std::unique_ptr<QGUploader>(QGUploader::CreateUploader(YAML::Load("{type: scp, host: " + sshHost + ", port: " + std::to_string(sshPort) + ", user: " + sshUser + ", dir: " + dir + "}"))));
	} else {
		_uploaders.push_back(std::unique_ptr<QGUploader>(QGUploader::CreateUploader(YAML::Load("{type: local, dir: " + dir + "}"))));
	}

	_init();
}

QrssPiG::QrssPiG(const std::string &configFile) : QrssPiG() {
	int iSampleRate = 48000;
	int pSampleRate = 6000;

	YAML::Node config = YAML::LoadFile(configFile);

	if (config["input"]) {
		if (config["input"].Type() != YAML::NodeType::Map) throw std::runtime_error("YAML: input must be a map");

		_inputDevice = QGInputDevice::CreateInputDevice(config["input"]);

		// Patch real samplerate and basefreq back to config so image class will have the correct value
		// Same for remaining ppm
		config["input"]["samplerate"] = _inputDevice->sampleRate();
		config["input"]["basefreq"] = _inputDevice->baseFreq();
		config["input"]["ppm"] = _inputDevice->ppm();

		iSampleRate = config["input"]["samplerate"].as<int>();
	}

	if (config["processing"]) {
		if (config["processing"].Type() != YAML::NodeType::Map) throw std::runtime_error("YAML: processing must be a map");

		YAML::Node processing = config["processing"];

		if (processing["chunksize"]) _chunkSize = processing["chunksize"].as<int>();
		if (processing["samplerate"]) pSampleRate = processing["samplerate"].as<int>();
		if (processing["fft"]) _N = processing["fft"].as<int>();
		if (processing["fftoverlap"]) {
			int o = processing["fftoverlap"].as<int>();
			if ((o < 0) || (o >= _N)) throw std::runtime_error("YAML: overlap value out of range [0..N[");
			_overlap = (o * _N) / (o + 1);
		}
	}

	if (pSampleRate && (iSampleRate != pSampleRate)) {
		_resampler.reset(new QGProcessor((float)iSampleRate/(float)pSampleRate, _chunkSize));

		// Patch config with real samplerate from resampler
		pSampleRate = iSampleRate / _resampler->getRealRate();
		if (config["processing"]) config["processing"]["samplerate"] = pSampleRate;
	}

	// Add provision to add a full chunksize when only one sample left before reaching N. If resampling, will be smaller in worst case
	_input.reset(new std::complex<float>[_N + _chunkSize - 1]);
	_inputIndex = 0;

	// Could be inlined, should be part of a processing init
	_init();

	if (config["output"]) {
		if (config["output"].Type() == YAML::NodeType::Map) _im.reset(new QGImage(config, 0));
		// else if (config["output"].Type() == YAML::NodeType::Sequence) for (unsigned int i = 0; i < config["output"].size(); i++) _im.reset(new QGImage((config, i));
		else throw std::runtime_error("YAML: output must be a map or a sequence");
	}

	if (config["upload"]) {
		if (config["upload"].IsMap()) _uploaders.push_back(std::move(QGUploader::CreateUploader(config["upload"])));
		else if (config["upload"].IsSequence()) for (YAML::const_iterator it = config["upload"].begin(); it != config["upload"].end(); it++) _uploaders.push_back(std::move(QGUploader::CreateUploader(*it)));
		else throw std::runtime_error("YAML: upload must be a map or a sequence");
	}
}

QrssPiG::~QrssPiG() {
	// Draw residual data if any
	try {
		_im->addLine(_fftOut);
	} catch (const std::exception &e) {};

	if (_fft) delete _fft;
}

void QrssPiG::run() {
	_inputDevice->setCb(std::bind(&QrssPiG::_addIQ, this, _1, _2), _chunkSize);
	for (auto&& uploader: _uploaders) _im->addCb(std::bind(&QGUploader::push, uploader, _1, _2, _3, _4, _5));
	
	_inputDevice->run();
}

void QrssPiG::stop() {
	_inputDevice->stop();
}

//
// Private members
//
void QrssPiG::_init() {
	_hannW.reset(new float[_N]);
	for (int i = 0; i < _N/2; i++) {
		_hannW[i] = .5 * (1 - cos((2 * M_PI * i) / (_N / 2 - 1)));
	}

	_fft = new QGFft(_N);
	_fftIn = _fft->getInputBuffer();
	_fftOut = _fft->getFftBuffer();

	_frameIndex = 0;
}

void QrssPiG::_addIQ(const std::complex<float> *iq, unsigned int len) {
	switch (len) {
	case 1:
std::cout << "Old api" << std::endl;
/*
		_input[_inputIndex++] = *iq;

		if (_input >= _inputIndexThreshold) {
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
*/
		break;

	case 32:
		if (_resampler) {
			_inputIndex += _resampler->processChunk(iq, _input.get() + _inputIndex);
		} else {
			std::memcpy(_input.get() + _inputIndex, iq, _chunkSize);
			_inputIndex += _chunkSize;
		}

		if (_inputIndex >= _N) {
			_computeFft();

			// Shift overlap to buffer start
			for (auto i = 0; i < _overlap + _inputIndex - _N; i++) _input[i] = _input[_N - _overlap + i];
			_inputIndex = _overlap + _inputIndex - _N;
		}
		break;

	default:
		throw std::runtime_error("Chunksize not supported");
	}
}

void QrssPiG::_computeFft() {
	for (int i = 0; i < _N; i++) _fftIn[i] = _input.get()[i] * _hannW[i / 2];
	_fft->process();
	_im->addLine(_fftOut);
}
