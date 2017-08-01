#pragma once

#include <string>

#include "QGUploader.h"

class QGSCPUploader: public QGUploader {
public:
	QGSCPUploader(const std::string &host = "localhost", const std::string &user = "", const std::string &dir = "./", int port = 0);
	~QGSCPUploader();

private:
	void _pushThreadImpl(const std::string &fileName, const char *data, int dataSize);

	std::string _host;
	std::string _user;
	std::string _dir;
	int _port;
	int _fileMode;
};
