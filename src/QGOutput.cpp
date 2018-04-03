#include "QGOutput.h"
#include "Config.h"

#include <iostream>
#include <stdexcept>

#include "QGPlugin.h"

std::unique_ptr<QGOutput> QGOutput::CreateOutput(const YAML::Node &config, const QGProcessor& processor, int index) {
	QGPlugin<QGOutput>* plugin = new QGPlugin<QGOutput>("output");
	QGOutput *obj = plugin->create(config, processor, index);

	return std::unique_ptr<QGOutput>(obj);
}

void QGOutput::addCb(std::function<void(const QGImage*, const char*, int, bool, bool)>cb) {
	_cbs.push_back(cb);
}
