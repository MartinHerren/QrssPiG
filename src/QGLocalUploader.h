#pragma once

#include <string>

#include "QGUploader.h"

class QGLocalUploader: public QGUploader {
public:
	QGLocalUploader(const std::string &dir = "./");
	~QGLocalUploader();

	void push(const std::string &fileName, const char *data, int dataSize);

private:
	std::string _dir;
};
