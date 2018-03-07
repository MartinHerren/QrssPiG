#include "QGOutput.h"
#include "Config.h"

#include <iostream>
#include <stdexcept>

#include "QGImage.h"

std::unique_ptr<QGOutput> QGOutput::CreateOutput(const YAML::Node &config, const QGProcessor& processor, int index) {
	if (!config["type"] || (config["type"].as<std::string>().compare("image") == 0)) {
		std::cout << "Output type image" << std::endl;
		return std::unique_ptr<QGImage>(new QGImage(config, processor, index));
	} else {
		throw std::runtime_error(std::string("QGOutput: unknown type ") + config["type"].as<std::string>());
	}
}

void QGOutput::addCb(std::function<void(const QGImage*, const char*, int, bool, bool)>cb) {
	_cbs.push_back(cb);
}
