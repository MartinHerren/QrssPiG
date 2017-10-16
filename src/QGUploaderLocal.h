#pragma once

#include <string>

#include "QGUploader.h"

class QGUploaderLocal: public QGUploader {
public:
	QGUploaderLocal(const YAML::Node &config);
	~QGUploaderLocal();

private:
	void _pushThreadImpl(const std::string &fileName, const char *data, int dataSize, std::string &uri);

	std::string _dir;
};
