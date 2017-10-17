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
	uri = std::string("file://") + _dir + (_dir.length() ? "/" : "") + fileName;

	std::ofstream o;
	o.open(_dir + (_dir.length() ? "/" : "") + fileName, std::ios::binary);

	if (!o.is_open()) throw std::runtime_error(std::string("Unable to write file ") + uri);

	o.write(data, dataSize);

	o.close();
}
