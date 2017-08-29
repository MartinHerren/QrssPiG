#pragma once

#include <string>

class QGUploader {
protected:
	QGUploader(bool pushIntermediate): _pushIntermediate(pushIntermediate) {};

public:
	virtual ~QGUploader() {};

	void pushIntermediate(const std::string &fileName, const char *data, int dataSize);
	void push(const std::string &fileName, const char *data, int dataSize, bool wait = false);

private:
	void _pushThread(std::string fileName, const char *data, int dataSize);

	virtual void _pushThreadImpl(const std::string &fileName, const char *data, int dataSize, std::string &uri) = 0;

	bool _pushIntermediate;
};
