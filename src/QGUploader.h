#pragma once

#include <string>

class QGUploader {
protected:
	QGUploader() {};

public:
	virtual ~QGUploader() {};

	virtual void push(const std::string &fileName, const char *data, int dataSize) = 0;
};
