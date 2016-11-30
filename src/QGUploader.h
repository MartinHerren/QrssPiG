#pragma once

#include <string>
#include <libssh/libssh.h>

class QGUploader {
public:
	QGUploader();
	~QGUploader();
	
	void pushFile(const std::string &fileName, const char *data, int dataSize);
	
private:
};
