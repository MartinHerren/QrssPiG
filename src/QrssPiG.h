#pragma once

#include <memory>
#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "QGInputDevice.h"
#include "QGProcessor.h"
#include "QGOutput.h"
#include "QGUploader.h"

class QrssPiG {
public:
	static void listModules();
	static void listDevices();

	QrssPiG(const std::string &configFile);
	~QrssPiG();

	void run();
	void stop();

private:
	std::shared_ptr<QGInputDevice> _inputDevice;
	std::shared_ptr<QGProcessor> _processor;
	std::vector<std::shared_ptr<QGOutput>> _outputs;
	std::vector<std::shared_ptr<QGUploader>> _uploaders;
};
