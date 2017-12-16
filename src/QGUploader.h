#pragma once

#include <memory>
#include <string>

#include <yaml-cpp/yaml.h>

class QGUploader {
public:
	static std::vector<std::string> listModules();

protected:
	QGUploader(const YAML::Node &config);

public:
	virtual ~QGUploader() {};

	void push(const std::string &fileName, const char *data, int dataSize, bool intermediate = false, bool wait = false);

	static std::unique_ptr<QGUploader> CreateUploader(const YAML::Node &config);

private:
	void _pushThread(std::string fileName, const char *data, int dataSize);

	virtual void _pushThreadImpl(const std::string &fileName, const char *data, int dataSize, std::string &uri) = 0;

protected:
	bool _verbose;
	bool _pushIntermediate;
	std::string _fileName;
};
