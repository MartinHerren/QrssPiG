#include "QGUploaderFTP.h"

#include <cstring>
#include <iostream>
#include <stdexcept>

#include <curl/curl.h>

QGUploaderFTP::QGUploaderFTP(const YAML::Node &config) : QGUploader(config) {
	_ssl = SSL::None;
	_insecure = false;
	_host = "localhost";
	_port = 21;
	_user = "";
	_password = "";
	_dir = "";
	_fileMode = 0644;

	if (config["type"].as<std::string>().compare("ftps") == 0) {
		// Shortcut for implicit ftp on port 990, later can still be overriden with port option
		_ssl = SSL::Implicit;
		_port = 990;
	}

	if (config["ssl"]) {
		std::string ssl = config["ssl"].as<std::string>();

		if (ssl.compare("none") == 0) {
			_ssl = SSL::None;
		} else if (ssl.compare("implicit") == 0) {
			_ssl = SSL::Implicit;
			_port = 990;
		} else if (ssl.compare("explicit") == 0) {
			_ssl = SSL::Explicit;
		} else {
			throw std::runtime_error("invalid ftp ssl option value");
		}
	}

	if (config["insecure"]) _insecure = config["insecure"].as<bool>();
	if (config["disableepsv"]) _disableEpsv = config["disableepsv"].as<bool>();

	if (config["host"]) _host = config["host"].as<std::string>();
	if (config["port"]) _port = config["port"].as<int>();
	if (config["user"]) _user = config["user"].as<std::string>();
	if (config["password"]) _password = config["password"].as<std::string>();
	if (config["dir"]) _dir = config["dir"].as<std::string>();

	CURLcode res;
	res = curl_global_init(CURL_GLOBAL_DEFAULT);

	if (res != CURLE_OK) {
		throw std::runtime_error(std::string("curl_global_init() failed: ") + curl_easy_strerror(res));
	}
}

QGUploaderFTP::~QGUploaderFTP() {
	curl_global_cleanup();
}

void QGUploaderFTP::_pushThreadImpl(const std::string &fileName, const char *data, int dataSize, std::string &uri) {
	CURL *curl;
	CURLcode res;

	struct FTPUploadData uploadData;

	uploadData.readptr = data;
	uploadData.sizeleft = dataSize;

	curl = curl_easy_init();

	if (!curl) {
		throw std::runtime_error("curl_easy_init() failed");
	}

	uri = (_ssl == SSL::Implicit ? std::string("ftps://") : std::string("ftp://")) + _host + ":" + std::to_string(_port) + "/" + _dir + (_dir.length() ? "/" : "") + fileName;

	curl_easy_setopt(curl, CURLOPT_URL, uri.c_str());
	curl_easy_setopt(curl, CURLOPT_USERPWD, (_user + ":" + _password).c_str());
	if (_ssl == SSL::Explicit) curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
	if (_insecure) curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // Needed to accept self-signed certs
	// if () curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L); // TODO: Would be needed to accept certs with missmatching host name
	if (_disableEpsv) curl_easy_setopt(curl, CURLOPT_FTP_USE_EPSV, 0L); // Needed to pass some routers/firewalls

	curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
	// we want to use our own read function
	curl_easy_setopt(curl, CURLOPT_READFUNCTION, QGUploaderFTP::cb);
	// pointer to pass to our read function
	curl_easy_setopt(curl, CURLOPT_READDATA, &uploadData);

	if (_verbose) curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

	// Set the expected upload size
	curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)uploadData.sizeleft);

	// Perform the request, res will get the return code
	res = curl_easy_perform(curl);

	if (res != CURLE_OK) {
		throw std::runtime_error(std::string("curl_easy_perform() failed: ") + curl_easy_strerror(res));
	}

	curl_easy_cleanup(curl);
}

size_t QGUploaderFTP::cb(void *ptr, size_t size, size_t nmemb, void *userp) {
	struct FTPUploadData *uploadData = (struct FTPUploadData *)userp;
	size_t max = size*nmemb;

	if (max < 1) return 0;

	if (uploadData->sizeleft) {
		size_t copylen = max;

		if (copylen > uploadData->sizeleft)
			copylen = uploadData->sizeleft;
		memcpy(ptr, uploadData->readptr, copylen);
		uploadData->readptr += copylen;
		uploadData->sizeleft -= copylen;
		return copylen;
	}

	return 0;
}
