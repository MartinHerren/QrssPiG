#pragma once

#include <string>

#include "QGUploader.h"

class QGUploaderFTP: public QGUploader {
public:
	QGUploaderFTP(const YAML::Node &config);
	~QGUploaderFTP();

private:
	enum class SSL { None, Implicit, Explicit };
	struct FTPUploadData { const char *readptr; size_t sizeleft; };

	void _pushThreadImpl(const std::string &fileName, const char *data, int dataSize, std::string &uri);

	static size_t cb(void *ptr, size_t size, size_t nmemb, void *userp);

	SSL _ssl;
	std::string _host;
	int _port;
	std::string _user;
	std::string _password;
	std::string _dir;
	int _fileMode;
};
