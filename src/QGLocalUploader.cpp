#include "QGLocalUploader.h"

#include <fstream>
#include <iostream>
#include <stdexcept>

QGLocalUploader::QGLocalUploader(const std::string &dir) :
	QGUploader(),
	_dir(dir) {
}

QGLocalUploader::~QGLocalUploader() {
}

void QGLocalUploader::_pushThreadImpl(const std::string &fileName, const char *data, int dataSize) {
	std::ofstream o;

	o.open(_dir + "/" + fileName, std::ios::binary);
	o.write(data, dataSize);
	o.close();
}
