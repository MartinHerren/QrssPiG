#include "QGUploaderSCP.h"

#include <iostream>
#include <stdexcept>

extern "C" QGUploader* create_object(const YAML::Node &config) {
        return new QGUploaderSCP(config);
}

QGUploaderSCP::QGUploaderSCP(const YAML::Node &config) : QGUploader(config) {
	_host = "localhost";
	_port = 22;
	_user = "";
	_dir = ".";
	_fileMode = 0644;

	if (config["host"]) _host = config["host"].as<std::string>();
	if (config["port"]) _port = config["port"].as<int>();
	if (config["user"]) _user = config["user"].as<std::string>();
	if (config["dir"]) _dir = config["dir"].as<std::string>();
}

QGUploaderSCP::~QGUploaderSCP() {
}

void QGUploaderSCP::_pushThreadImpl(const std::string &fileName, const char *data, int dataSize, std::string &uri) {
	ssh_session ssh;

	uri = std::string("ssh://") + (_user.length() ? _user + "@" : "") + _host + ":" + _dir + (_dir.length() ? "/" : "") + fileName;

	ssh = ssh_new();

	if (ssh == NULL) throw std::runtime_error("Error allocating ssh handler");

	ssh_options_set(ssh, SSH_OPTIONS_HOST, _host.c_str());
	if (_user.length() > 0) ssh_options_set(ssh, SSH_OPTIONS_USER, _user.c_str());
	if (_port > 0) ssh_options_set(ssh, SSH_OPTIONS_PORT, &_port);

	int verbosity = SSH_LOG_PROTOCOL;
	if (_verbose) ssh_options_set(ssh, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);

	if (ssh_connect(ssh) != SSH_OK) {
		ssh_free(ssh);
		throw std::runtime_error("Error connecting to server");
	}

	// Todo: improve auth host
	switch (ssh_is_server_known(ssh)) {
	case SSH_SERVER_KNOWN_OK:
		break;

	case SSH_SERVER_KNOWN_CHANGED:
	case SSH_SERVER_FOUND_OTHER:
		std::cout << "Server key or key type changed" << std::endl << std::endl;
		break;

	case SSH_SERVER_NOT_KNOWN:
	case SSH_SERVER_FILE_NOT_FOUND:
		_getServerHash(&ssh);
		ssh_write_knownhost(ssh);
		break;

	case SSH_SERVER_ERROR:
		throw std::runtime_error("Error verifying server known");
	}

	// Todo: improve user auth
	ssh_userauth_publickey_auto(ssh, NULL, NULL);

	ssh_scp scp = ssh_scp_new(ssh, SSH_SCP_WRITE, _dir.c_str());

	if (scp == NULL) {
		ssh_disconnect(ssh);
		ssh_free(ssh);
		throw std::runtime_error("Error creating scp");
	}

	if (ssh_scp_init(scp) != SSH_OK) {
		ssh_scp_free(scp);
		ssh_disconnect(ssh);
		ssh_free(ssh);
		throw std::runtime_error("Error initializing scp");
	}

	if (ssh_scp_push_file(scp, fileName.c_str(), dataSize, _fileMode) != SSH_OK) {
		ssh_scp_close(scp);
		ssh_scp_free(scp);
		ssh_disconnect(ssh);
		ssh_free(ssh);
		throw std::runtime_error("Error uploading file over scp");
	}

	if (ssh_scp_write(scp, data, dataSize) != SSH_OK) {
		ssh_scp_close(scp);
		ssh_scp_free(scp);
		ssh_disconnect(ssh);
		ssh_free(ssh);
		throw std::runtime_error("Error saving file over scp");
	}

	ssh_scp_close(scp);
	ssh_scp_free(scp);
	ssh_disconnect(ssh);
	ssh_free(ssh);
}

void QGUploaderSCP::_getServerHash(ssh_session *session) {
	ssh_key serverPublicKey;

	if (ssh_get_publickey(*session, &serverPublicKey) != SSH_OK) {
		throw std::runtime_error("Error getting server public key");
	}

	unsigned char *hash = NULL;
	size_t hashLen = 0;

	if (ssh_get_publickey_hash(serverPublicKey, SSH_PUBLICKEY_HASH_SHA1, &hash, &hashLen) != SSH_OK) {
		throw std::runtime_error("Error getting hash from server public key");
	}

//	ssh_print_hexa("hash", hash, hashLen);

	ssh_clean_pubkey_hash(&hash);
}
