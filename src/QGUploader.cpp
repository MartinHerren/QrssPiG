#include "QGUploader.h"

#include <iostream>

QGUploader::QGUploader() {
}

QGUploader::~QGUploader() {
}

void QGUploader::pushFile(const std::string &fileName) {
	ssh_session ssh;
	
	std::string host("localhost");
	int port = 11235;
	//std::string user("user");
	int verbosity = SSH_LOG_PROTOCOL;
	char b[100];
	int rc;

	ssh = ssh_new();

	if (ssh == NULL) {
		// Throw
		return;
	}
	
std::cout << "ssh created" << std::endl;

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
	
std::cout << "ssh connected" << std::endl;

	ssh_scp scp = ssh_scp_new(ssh, SSH_SCP_WRITE, ".");

	if (scp == NULL) {
		// Throw
		ssh_disconnect(ssh);
		ssh_free(ssh);
		return;
	}
std::cout << "scp created" << std::endl;

	rc = ssh_scp_init(scp);

	if (rc != SSH_OK) {
std::cout << "scp init failed: " << ssh_get_error(ssh) << std::endl;
		// Throw
		ssh_scp_free(scp);
		ssh_disconnect(ssh);
		ssh_free(ssh);
		return;
	}
std::cout << "scp initialized" << std::endl;

	rc = ssh_scp_push_file(scp, fileName.c_str(), 100, 0644);

	if (rc != SSH_OK) {
		// Throw
	ssh_scp_close(scp);
		ssh_scp_free(scp);
		ssh_disconnect(ssh);
		ssh_free(ssh);
		return;
	}
std::cout << "scp pushed" << std::endl;

	rc = ssh_scp_write(scp, b, 100);

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
