#pragma once

#include <string>

class QGUploader {
protected:
	QGUploader() {};

public:
	virtual ~QGUploader() {};

	void push(const std::string &fileName, const char *data, int dataSize, bool wait = false);

private:
	void _pushThread(std::string fileName, const char *data, int dataSize);

	virtual void _pushThreadImpl(const std::string &fileName, const char *data, int dataSize) = 0;
};
