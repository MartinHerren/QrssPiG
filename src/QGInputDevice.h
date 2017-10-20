#pragma once

#include <functional>

#include <yaml-cpp/yaml.h>

class QGInputDevice {
protected:
	QGInputDevice(const YAML::Node &config);

public:
	virtual ~QGInputDevice() {};

	virtual void open() = 0;
	
	virtual void run(std::function<void(std::complex<float>)>cb) { (void)cb; };
	virtual void stop() {};

	int sampleRate() { return _sampleRate; };
	int baseFreq() { return _baseFreq; };

	static QGInputDevice *CreateInputDevice(const YAML::Node &config);

protected:
	int _sampleRate;
	int _baseFreq;
};
