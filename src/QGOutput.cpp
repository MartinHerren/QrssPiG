#include "QGOutput.h"
#include "Config.h"

#include <iostream>
#include <stdexcept>

#include "QGImage.h"

std::unique_ptr<QGOutput> QGOutput::CreateOutput(const YAML::Node &config, unsigned int index) {
	if (config["output"]) {
		YAML::Node output;

		// Get the correct output when having multiple outputs
		if (config["output"].IsMap()) output = config["output"];
		else if (config["output"].IsSequence()) output = config["output"][index];

	    if (!output["type"] || (output["type"].as<std::string>().compare("image") == 0)) {
	        std::cout << "Output type image" << std::endl;
	        return std::unique_ptr<QGImage>(new QGImage(config, index));
	    } else {
	        throw std::runtime_error(std::string("QGOutput: unknown type ") + output["type"].as<std::string>());
	    }
	}

	// Fall back with default image type
	return std::unique_ptr<QGImage>(new QGImage(config, index));
}

void QGOutput::addCb(std::function<void(const std::string&, const std::string&, long int, std::chrono::milliseconds, const char*, int, bool, bool)>cb) {
	_cbs.push_back(cb);
}
