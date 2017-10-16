#include "QGUploaderLocal.h"

#include <fstream>
#include <iostream>
#include <stdexcept>

QGUploaderLocal::QGUploaderLocal(const YAML::Node &config) : QGUploader(config) {
	_dir = "";

	if (config["dir"]) _dir = config["dir"].as<std::string>();
}

QGUploaderLocal::~QGUploaderLocal() {
}

void QGUploaderLocal::_pushThreadImpl(const std::string &fileName, const char *data, int dataSize, std::string &uri) {
	std::ofstream o;

	uri = std::string("file://") + _dir + (_dir.length() ? "/" : "") + fileName;

	o.open(uri, std::ios::binary);
	o.write(data, dataSize);
	o.close();
}
