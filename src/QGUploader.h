#pragma once

#include <string>
#include <yaml-cpp/yaml.h>

class QGUploader {
protected:
	QGUploader(const YAML::Node &config);

public:
	virtual ~QGUploader() {};

	void pushIntermediate(const std::string &fileName, const char *data, int dataSize);
	void push(const std::string &fileName, const char *data, int dataSize, bool wait = false);

	static QGUploader *CreateUploader(const YAML::Node &config);

private:
	void _pushThread(std::string fileName, const char *data, int dataSize);

	virtual void _pushThreadImpl(const std::string &fileName, const char *data, int dataSize, std::string &uri) = 0;

protected:
	bool _verbose;
	bool _pushIntermediate;
};
