#pragma once

#include <string>

class QGUploader {
public:
	QGUploader(const std::string &sshHost = "localhost", const std::string &sshUser = "", const std::string &sshDir = ".", int sshPort = 0);
	~QGUploader();

	void pushFile(const std::string &fileName, const char *data, int dataSize);

private:
	std::string _sshHost;
	std::string _sshUser;
	std::string _sshDir;
	int _sshPort;
	int _fileMode;
};
