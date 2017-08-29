#pragma once

#include <string>

#include "QGUploader.h"

class QGLocalUploader: public QGUploader {
public:
	QGLocalUploader(bool pushIntermediate, const std::string &dir = "./");
	~QGLocalUploader();

private:
	void _pushThreadImpl(const std::string &fileName, const char *data, int dataSize, std::string &uri);

	std::string _dir;
};
