#pragma once

#include <chrono>
#include <complex>
#include <functional>
#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "QGProcessor.h"

class QGImage;

class QGOutput {
public:
	static std::unique_ptr<QGOutput> CreateOutput(const YAML::Node &config, const QGProcessor& processor, int index = -1);

protected:
	QGOutput(const YAML::Node &config, const QGProcessor& processor, int index) : processor(processor), index(index) { (void)config; };

public:
	virtual ~QGOutput() {};

	void addCb(std::function<void(const QGImage*, const char*, int, bool, bool)>cb);

	virtual void addLine(const std::complex<float> *fft) = 0;

	const QGProcessor &processor;
	const int index;

protected:
	std::vector<std::function<void(const QGImage*, const char*, int, bool, bool)>> _cbs;
};
