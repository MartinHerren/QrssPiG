#pragma once

#include <memory>
#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "QGInputDevice.h"
#include "QGProcessor.h"
#include "QGImage.h"
#include "QGUploader.h"

class QrssPiG {
public:
	static void listModules();
	static void listDevices();

	QrssPiG(const std::string &format, int sampleRate, int N, const std::string &dir, const std::string &sshHost, const std::string &sshUser, int sshPort);
	QrssPiG(const std::string &configFile);
	~QrssPiG();

	void run();
	void stop();

private:
	std::shared_ptr<QGInputDevice> _inputDevice;
	std::shared_ptr<QGProcessor> _processor;
	std::shared_ptr<QGImage> _image;
	std::vector<std::shared_ptr<QGUploader>> _uploaders;
};
