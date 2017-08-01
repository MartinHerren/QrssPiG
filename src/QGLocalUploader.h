#pragma once

#include <string>

#include "QGUploader.h"

class QGLocalUploader: public QGUploader {
public:
	QGLocalUploader(const std::string &dir = "./");
	~QGLocalUploader();

private:
	void _pushThreadImpl(const std::string &fileName, const char *data, int dataSize);
	
	std::string _dir;
};
