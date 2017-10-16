#include "QGUploaderLocal.h"

#include <fstream>
#include <iostream>
#include <stdexcept>

QGUploaderLocal::QGUploaderLocal(const std::string &dir) :
	QGUploader(),
	_dir(dir) {
}

QGUploaderLocal::~QGUploaderLocal() {
}

void QGUploaderLocal::_pushThreadImpl(const std::string &fileName, const char *data, int dataSize, std::string &uri) {
	std::ofstream o;

	uri = _dir + "/" + fileName;

	o.open(uri, std::ios::binary);
	o.write(data, dataSize);
	o.close();
}
