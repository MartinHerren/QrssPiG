#include "QGUploader.h"

#include <iostream>
#include <libssh/libssh.h>
//#include <libssh/libsshpp.hpp> // Not available in debian jessie, available in stretch

QGUploader::QGUploader(const std::string &sshHost, const std::string &sshUser, int sshPort): _sshHost(sshHost), _sshUser(sshUser), _sshPort(sshPort) {
}

QGUploader::~QGUploader() {
}

void QGUploader::pushFile(const std::string &fileName, const char *data, int dataSize) {
	ssh_session ssh;
	
	int verbosity = SSH_LOG_PROTOCOL;
	int rc;

	ssh = ssh_new();

	if (ssh == NULL) {
		// Throw
		return;
	}

	ssh_options_set(ssh, SSH_OPTIONS_HOST, _sshHost.c_str());
	if (_sshUser.length() > 0) ssh_options_set(ssh, SSH_OPTIONS_USER, _sshUser.c_str());
	if (_sshPort > 0) ssh_options_set(ssh, SSH_OPTIONS_PORT, &_sshPort);
	//ssh_options_set(ssh, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);
	
	rc = ssh_connect(ssh);

	if (rc != SSH_OK) {
		// Throw
		ssh_disconnect(ssh);
		ssh_free(ssh);
		return;
	}
	
	// Todo: improve auth host
	ssh_is_server_known(ssh);
	unsigned char *hash = NULL;
	ssh_get_pubkey_hash(ssh, &hash);
	ssh_write_knownhost(ssh);
	
	// Todo: improve user auth
	ssh_userauth_publickey_auto(ssh, NULL, NULL);

	ssh_scp scp = ssh_scp_new(ssh, SSH_SCP_WRITE, ".");

	if (scp == NULL) {
		// Throw
		ssh_disconnect(ssh);
		ssh_free(ssh);
		return;
	}

	rc = ssh_scp_init(scp);

	if (rc != SSH_OK) {
		// Throw
		ssh_scp_free(scp);
		ssh_disconnect(ssh);
		ssh_free(ssh);
		return;
	}
std::cout << "scp initialized" << std::endl;

	rc = ssh_scp_push_file(scp, fileName.c_str(), dataSize, 0644);

	if (rc != SSH_OK) {
		// Throw
	ssh_scp_close(scp);
		ssh_scp_free(scp);
		ssh_disconnect(ssh);
		ssh_free(ssh);
		return;
	}
std::cout << "scp pushed" << std::endl;

	rc = ssh_scp_write(scp, data, dataSize);

	if (rc != SSH_OK) {
		// Throw
	ssh_scp_close(scp);
		ssh_scp_free(scp);
		ssh_disconnect(ssh);
		ssh_free(ssh);
		return;
	}
std::cout << "scp written" << std::endl;

	ssh_scp_close(scp);

	ssh_scp_free(scp);
	
	ssh_disconnect(ssh);
	ssh_free(ssh);
}
