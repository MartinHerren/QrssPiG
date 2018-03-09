#include "QGOutput.h"
#include "Config.h"

#include <iostream>
#include <stdexcept>

#include "QGPlugin.h"

std::unique_ptr<QGOutput> QGOutput::CreateOutput(const YAML::Node &config, const QGProcessor& processor, int index) {
	if (config["output"]) {
		YAML::Node output;

		// Get the correct output when having multiple outputs
		if (config["output"].IsMap()) output = config["output"];
		else if (config["output"].IsSequence()) output = config["output"][index];

		QGPlugin<QGOutput>* plugin = new QGPlugin<QGOutput>("output");

		QGOutput *obj = plugin->create(output, processor, index);

		return std::unique_ptr<QGOutput>(obj);
	}

	throw std::runtime_error(std::string("QGOutput: No output type defined"));
}

void QGOutput::addCb(std::function<void(const QGImage*, const char*, int, bool, bool)>cb) {
	_cbs.push_back(cb);
}
