#pragma once

#include <functional>

#include <yaml-cpp/yaml.h>

class QGInputDevice {
protected:
	QGInputDevice(const YAML::Node &config);

public:
	virtual ~QGInputDevice() {};

	virtual void run(std::function<void(std::complex<float>)>cb) { (void)cb; };
	virtual void stop() {};

	unsigned int sampleRate() { return _sampleRate; };
	unsigned int baseFreq() { return _baseFreq; };

	static QGInputDevice *CreateInputDevice(const YAML::Node &config);

protected:
	unsigned int _sampleRate;
	unsigned int _baseFreq;
};
