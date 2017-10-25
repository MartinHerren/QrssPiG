#pragma once

#include <functional>

#include <yaml-cpp/yaml.h>

class QGInputDevice {
protected:
	QGInputDevice(const YAML::Node &config);

public:
	virtual ~QGInputDevice() {};

	virtual void run(std::function<void(std::complex<float>)>cb) = 0;
	virtual void stop() = 0;

	unsigned int sampleRate() { return _sampleRate; };
	unsigned int baseFreq() { return _baseFreq; };
	int ppm() { return _ppm; };

	static QGInputDevice *CreateInputDevice(const YAML::Node &config);

protected:
	unsigned int _sampleRate;
	unsigned int _baseFreq;
	int _ppm;
};
