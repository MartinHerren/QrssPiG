#pragma once

#include <functional>
#include <memory>

#include <yaml-cpp/yaml.h>

class QGInputDevice {
protected:
	QGInputDevice(const YAML::Node &config, std::function<void(std::complex<float>)>cb);

public:
	virtual ~QGInputDevice() {};

	virtual void run() = 0;
	virtual void stop() = 0;

	unsigned int sampleRate() { return _sampleRate; };
	unsigned int baseFreq() { return _baseFreq; };
	int ppm() { return _ppm; };

	static std::unique_ptr<QGInputDevice> CreateInputDevice(const YAML::Node &config, std::function<void(std::complex<float>)>cb);

protected:
	unsigned int _sampleRate;
	unsigned int _baseFreq;
	int _ppm;

	std::function<void(std::complex<float>)> _cb;
};
