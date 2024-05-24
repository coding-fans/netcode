/*
 * Author: fasion
 * Created time: 2024-05-18 15:31:15
 * Last Modified by: fasion
 * Last Modified time: 2024-05-24 13:49:27
 */

#include <netdb.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
	// process exit_code, return by main
	int exit_code = 0;

	// use tls to connect https server and fetch data specifiled by the following url:
	// https://fasionchan.com/about.txt
	const char *service = "https";
	const char *host = "fasionchan.com";
	const char *path = "/about.txt";

	// hints for getaddrinfo
	// getaddrinfo will return resolved result as addrinfo
	// with default values given by hints
	struct addrinfo hints = { 0 };
	hints.ai_socktype = SOCK_STREAM;

	// call getaddrinfo to resolve domain name
	// returned data will be saved in variable result
	struct addrinfo *result;
	int retval = getaddrinfo(host, service, &hints, &result);
	if (retval != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(retval));
		return 1;
	}

	// tcp socket fd
	int sfd = 0;

	// try to connect resolved addresses one byone
	struct addrinfo *cursor;
	for (cursor = result; cursor != NULL; cursor = cursor->ai_next) {
		// create socket
		sfd = socket(cursor->ai_family, cursor->ai_socktype, cursor->ai_protocol);
		if (sfd == -1) {
			perror("socket");
			break;
		}

		// connect to the server, address is returned by getaddrinfo
		int retval = connect(sfd, cursor->ai_addr, cursor->ai_addrlen);
		if (retval == -1) {
			// if failed, try continue to try next
			perror("connect");
			close(sfd);
			continue;
		}

		break;
	}

	// release memory returned by getaddrinfo
	freeaddrinfo(result);

	if (sfd < 0) {
		return 2;
	}

	// init openssl lib
	SSL_library_init();
	SSL_load_error_strings();
	OpenSSL_add_all_algorithms();

	// create ssl context object
	SSL_CTX *sslctx = SSL_CTX_new(TLS_method());
	if (sslctx == NULL) {
		ERR_print_errors_fp(stderr);
		exit_code = 3;
		goto close_socket;
	}

	// use default pathes for ca-cert
	if (!SSL_CTX_set_default_verify_paths(sslctx)) {
		ERR_print_errors_fp(stderr);
		exit_code = 4;
		goto free_ssl_ctx;
	}

	// create ssl object
	SSL *ssl = SSL_new(sslctx);
	if (ssl == NULL) {
		ERR_print_errors_fp(stderr);
		exit_code = 5;
		goto free_ssl_ctx;
	}

	// set the bachend tcp socket
	if (!SSL_set_fd(ssl, sfd)) {
		ERR_print_errors_fp(stderr);
		exit_code = 6;
		goto free_ssl;
	}

	// initiate ssl connecting
	if (SSL_connect(ssl) != 1) {
		ERR_print_errors_fp(stderr);
		exit_code = 7;
		goto free_ssl;
	}

	// verify tls cert
	if (SSL_get_verify_result(ssl) != X509_V_OK) {
		ERR_print_errors_fp(stderr);
		exit_code = 8;
		goto shutdown_ssl;
	}

	// create BIO object for buffered reading/writing
	BIO *bio = BIO_new_buffer_ssl_connect(sslctx);
	if (bio == NULL) {
		ERR_print_errors_fp(stderr);
		exit_code = 9;
		goto shutdown_ssl;
	}

	// use ssl object as backend connection
	// then we can use bio to read/write ssl connection
	retval = BIO_set_ssl(bio, ssl, BIO_NOCLOSE);
	if (retval != 1) {
		ERR_print_errors_fp(stderr);
		exit_code = 10;
		goto free_bio;
	}

	char buffer[10240];

	// format http request
	int bytes_to_send = snprintf(buffer, sizeof(buffer), "GET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n", path, host);
	if (bytes_to_send < 0) {
		perror("snprintf");
		exit_code = 11;
		goto free_bio;
	}

	// call BIO_write to send http request until all bytes sent
	while (bytes_to_send > 0) {
		int bytes_sent = BIO_write(bio, buffer, bytes_to_send);
		if (bytes_sent < 0) {
			ERR_print_errors_fp(stderr);
			exit_code = 12;
			goto free_bio;
		}

		bytes_to_send -= bytes_sent;
	}

	// flush all buffered data to server, ensure that all data is sent
	if (BIO_flush(bio) != 1) {
		ERR_print_errors_fp(stderr);
		exit_code = 13;
		goto free_bio;
	}

	for (;;) {
		// read data into buffer if any
		int bytes_read = BIO_read(bio, buffer, sizeof(buffer));
		if (bytes_read < 0) {
			ERR_print_errors_fp(stderr);
			exit_code = 14;
			goto free_bio;
		} else if (bytes_read == 0) {
			break;
		}

		// write received data to stdout
		if (write(STDOUT_FILENO, buffer, bytes_read) < 0) {
			perror("write");
			exit_code = 15;
			goto free_bio;
		}
	}

free_bio:
	BIO_free_all(bio);

shutdown_ssl:
	// don't worry, SSL_shutdown can be called several times
	SSL_shutdown(ssl);

free_ssl:
	SSL_free(ssl);

free_ssl_ctx:
	SSL_CTX_free(sslctx);

close_socket:
	close(sfd);

	return exit_code;
}
