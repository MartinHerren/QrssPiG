#include "QrssPiG.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <syslog.h>

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
using std::placeholders::_4;
using std::placeholders::_5;

void QrssPiG::listModules() {
	std::cout << "Available input modules:" << std::endl;
	for (const auto&m: QGInputDevice::listModules()) std::cout << "\t" << m << std::endl;

	std::cout << "Available upload modules:" << std::endl;
	for (const auto&m: QGUploader::listModules()) std::cout << "\t" << m << std::endl;
}

void QrssPiG::listDevices() {
	auto devices = QGInputDevice::listDevices();

  	std::cout << "Available input devices:" << std::endl;
	for (const auto& m: devices) {
		if (m.second.size()) for (const auto &d: m.second) std::cout << "\t" << m.first << ":\t" << d << std::endl;
		else std::cout << "\t" << m.first << ":\tNo device found" << std::endl;
	}
}

QrssPiG::QrssPiG(const std::string &configFile) {
	YAML::Node config = YAML::LoadFile(configFile);

	syslog(LOG_INFO, "Configuring");

std::cout << "Configuring input" << std::endl;
	// Create input device
	if (config["input"] && config["input"].Type() != YAML::NodeType::Map)
		throw std::runtime_error("YAML: input must be a map");
	_inputDevice = QGInputDevice::CreateInputDevice(config["input"]);

std::cout << "Configuring processing" << std::endl;
	// Create processor
	if (config["processing"] && config["processing"].Type() != YAML::NodeType::Map)
		throw std::runtime_error("YAML: processing must be a map");
	_processor.reset(new QGProcessor(config["processing"], *(_inputDevice.get())));

	// Connect input to processor
	_inputDevice->setCb(std::bind(&QGProcessor::addIQ, _processor, _1), _processor->chunkSize());

std::cout << "Configuring output" << std::endl;
	// Create all outputs
	if (config["output"]) {
		if (config["output"].Type() == YAML::NodeType::Map)
			_outputs.push_back(QGOutput::CreateOutput(config["output"], *(_processor.get())));
		else if (config["output"].Type() == YAML::NodeType::Sequence)
			for (unsigned int i = 0; i < config["output"].size(); i++)
				_outputs.push_back(QGOutput::CreateOutput(config["output"][i], *(_processor.get()), i));
		else
			throw std::runtime_error("YAML: output must be a map or a sequence");
	}

	// Connect processor to every output
	for (auto&& output: _outputs) _processor->addCb(std::bind(&QGOutput::addLine, output, _1));

std::cout << "Configuring upload" << std::endl;
	// Create all uploader
	if (config["upload"]) {
		if (config["upload"].IsMap())
			_uploaders.push_back(QGUploader::CreateUploader(config["upload"]));
		else if (config["upload"].IsSequence())
			for (YAML::const_iterator it = config["upload"].begin(); it != config["upload"].end(); it++)
				_uploaders.push_back(QGUploader::CreateUploader(*it));
		else throw std::runtime_error("YAML: upload must be a map or a sequence");
	}

	// Connect every output to every uploader
	for (auto&& uploader: _uploaders)
		for (auto&& output: _outputs)
			output->addCb(std::bind(&QGUploader::push, uploader, _1, _2, _3, _4, _5));

std::cout << "Configured" << std::endl;
}

QrssPiG::~QrssPiG() {
}

void QrssPiG::run() {
	syslog(LOG_INFO, "Started");
	std::cout << "Started" << std::endl;
	_inputDevice->run();
}

void QrssPiG::stop() {
	syslog(LOG_INFO, "Stopping");
	std::cout << "Stopping" << std::endl;
	_inputDevice->stop();
	syslog(LOG_INFO, "Stopped");
	std::cout << "Stopped" << std::endl;
}
