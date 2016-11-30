#include "QGUploader.h"

#include <iostream>

QGUploader::QGUploader() {
}

QGUploader::~QGUploader() {
}

void QGUploader::pushFile(const std::string &fileName, const char *data, int dataSize) {
	ssh_session ssh;
	
	std::string host("localhost");
	int port = 11235;
	//std::string user("user");
	int verbosity = SSH_LOG_PROTOCOL;
	int rc;

	ssh = ssh_new();

	if (ssh == NULL) {
		// Throw
		return;
	}

	ssh_options_set(ssh, SSH_OPTIONS_HOST, host.c_str());
	ssh_options_set(ssh, SSH_OPTIONS_PORT, &port);
	//ssh_options_set(ssh, SSH_OPTIONS_USER, user.c_str());
	ssh_options_set(ssh, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);
	
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
