#include "QGLocalUploader.h"

#include <fstream>
#include <iostream>
#include <stdexcept>

QGLocalUploader::QGLocalUploader(bool pushIntermediate, const std::string &dir) :
	QGUploader(pushIntermediate),
	_dir(dir) {
}

QGLocalUploader::~QGLocalUploader() {
}

void QGLocalUploader::_pushThreadImpl(const std::string &fileName, const char *data, int dataSize, std::string &uri) {
	std::ofstream o;

	uri = _dir + "/" + fileName;

	o.open(uri, std::ios::binary);
	o.write(data, dataSize);
	o.close();
}
