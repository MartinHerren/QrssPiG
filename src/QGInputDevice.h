#pragma once

#include <yaml-cpp/yaml.h>

class QGInputDevice {
protected:
	QGInputDevice(const YAML::Node &config);

public:
	virtual ~QGInputDevice() {};

	virtual void open() = 0;

	int sampleRate() { return _sampleRate; };
	int baseFreq() { return _baseFreq; };

	static QGInputDevice *CreateInputDevice(const YAML::Node &config);

protected:
	int _sampleRate;
	int _baseFreq;
};
