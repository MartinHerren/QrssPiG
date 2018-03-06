#pragma once

#include <chrono>
#include <complex>
#include <functional>
#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>

class QGOutput {
protected:
	QGOutput() {};

public:
	virtual ~QGOutput() {};

	static std::unique_ptr<QGOutput> CreateOutput(const YAML::Node &config, unsigned int index);

	void addCb(std::function<void(const std::string&, const std::string&, long int, std::chrono::milliseconds, const char*, int, bool, bool)>cb);

	virtual void addLine(const std::complex<float> *fft) = 0;

protected:
	std::vector<std::function<void(const std::string&, const std::string&, long int, std::chrono::milliseconds, const char*, int, bool, bool)>> _cbs;
};
