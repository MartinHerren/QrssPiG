#pragma once

#include <string>

#include <libssh/libssh.h>
//#include <libssh/libsshpp.hpp> // Not available in debian jessie, available in stretch

#include "QGUploader.h"

class QGUploaderSCP: public QGUploader {
public:
	QGUploaderSCP(const YAML::Node &config);
	~QGUploaderSCP();

private:
	void _pushThreadImpl(const std::string &fileName, const char *data, int dataSize, std::string &uri);

	void _getServerHash(ssh_session *session);

	std::string _host;
	std::string _user;
	std::string _dir;
	int _port;
	int _fileMode;
};
