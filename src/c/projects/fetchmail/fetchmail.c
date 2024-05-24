#include <argp.h>
#include <netdb.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 102400

#define ERR_CMDLINE_ARGS 1
#define ERR_CONNECTION 2
#define ERR_IMAP 3
#define ERR_DATA_PARSE 4
#define ERR_OTHER 5

#define ERR_NOT_FOUND 3

// log error info to stderr
int logerr(const char *format, ...) {
	va_list ap;
	va_start(ap, format);

	int bytes = vfprintf(stderr, format, ap);
	if (bytes < 0) {
		return bytes;
	}

	// write "\n" if needed
	if (format[strlen(format) - 1] != '\n') {
		int more_bytes = fprintf(stderr, "\n");
		if (more_bytes < 0) {
			return more_bytes;
		} else {
			bytes += more_bytes;
		}
	}

	return bytes;
}

// unfold headers
void unfold_headers(char *header, int header_bytes) {
	char *read = header;
	char *write = header;
	char *end = header + header_bytes;

	while (read < end) {
		// if meet CRLF + WSP, skip CRLF
		// WSP is sp(' '/0x20) or htab(0x09)
		if (read[0] == '\r' && read[1] == '\n' && (read[2] == ' ' || read[2] == 0x09)) {
			read += 2;
		}

		if (read != write) {
			*write = *read;
		}

		write++;
		read++;
	}

	*write = '\0';
}

char *dup_escape(char *str) {
	if (str==NULL) {
		return NULL;
	}

	char *result = malloc(2*strlen(str) + 1);
	if (result==NULL) {
		return NULL;
	}

	char *read = str;
	char *write = result;

	while (*read != '\0') {
		if (*read == '\"') {
			*write = '\\';
			write++;

			*write = '\"';
			write++;
		} else if (*read == '\r') {
			*write = '\\';
			write++;

			*write = 'r';
			write++;
		} else if (*read == '\n') {
			*write = '\\';
			write++;

			*write = 'n';
			write++;
		} else {
			*write = *read;
			write++;
		}

		read ++;
	}

	*write = '\0';

	return result;
}

// convert string to lower-case
void strlwr(char *str) {
	while(*str != '\0') {
		*str = tolower(*str);
		str++;
	}
}

int strempty(char *str) {
	return str==NULL || str[0]=='\0';
}

void safe_free(void *ptr) {
	logerr("[DEBUG] safe_free(ptr=%p)\n", ptr);

	if (ptr != NULL) {
		free(ptr);
	}
}

// message is used to save common headers and body
struct message {
	char *mime_version;
	char *from;
	char *date;
	char *subject;
	char *to;
	char *content_type;
	char *body;
	int body_bytes;
};

// parse raw message into struct message
// - buffer: the address of raw message, raw message if end of '\0'
// - length: the length of raw message, in bytes
int parse_message(char *buffer, int bytes, struct message *msg) {
	logerr("[DEBUG] parse_message(buffer=%p, bytes=%d, message=%p)\n", buffer, bytes, msg);

	// the first part of message is header
	char *header = buffer;
	int header_bytes = bytes;

	// delim between header and body: CRLFCRLF
	char *delim = strstr(buffer, "\r\n\r\n");
	logerr("[DEBUG] parse_message() delim=%p\n", delim);
	if (delim != NULL) {
		header_bytes = delim - buffer;
		*delim = '\0';

		// body found
		char *body = delim + 4;
		int body_bytes = bytes - (body - buffer);

		logerr("[DEBUG] parse_message() body=%p body_offset=%d body_bytes=%d\n", body, body-buffer, body_bytes);

		// save body
		msg->body = body;
		msg->body_bytes = body_bytes;
	}

	logerr("[DEBUG] parse_message() header_bytes=%d\n", header_bytes);

	unfold_headers(header, header_bytes);

	// parse headers line by line
	char *line = strtok(header, "\r\n");
	for ( ; line != NULL; line = strtok(NULL, "\r\n")) {
		// delim between header name and value is ':'
		delim = strstr(line, ":");
		if (delim == NULL) {
			continue;
		}

		*delim = '\0';
		strlwr(line);

		char *value = delim+1;
		while (*value == ' ') value++;

		if (strcmp(line, "content-type") == 0) {
			msg->content_type = value;
		} else if (strcmp(line, "mime-version") == 0) {
			msg->mime_version = value;
		} else if (strcmp(line, "from") == 0) {
			msg->from = value;
		} else if (strcmp(line, "date") == 0) {
			msg->date = value;
		} else if (strcmp(line, "subject") == 0) {
			char *value_end = value + strlen(value) -1;
			while (value_end >= value && *value_end == ' ') {
				*value_end = 0;
				value_end--;
			}

			msg->subject = value;
		} else if (strcmp(line, "to") == 0) {
			msg->to = value;
		}
	}

	return 0;
}

// struct context is used to save runtime context, includes:
// - command line arguments
// - tag
// - socket
// - ssl connection object
// - BIO object
struct context {
	char *username;
	char *password;
	char *folder;
	char *message_num;
	int use_tls;
	char *command;
	char *server_name;

	int tagno;
	char *tag; // tag for imap command

	int sfd; // fd of socket
	SSL_CTX *sslctx; // ssl context
	SSL *ssl; // ssl connection, if -t is specified
	BIO *bio; // BIO object for read/write data
};

int setup_tls(struct context *ctx);
int login_imap_server(struct context *ctx);
int send_command(struct context *ctx, const char *command, ...);

// generate next tag
char *next_tag(struct context *ctx) {
	ctx->tagno++;

	static char tag_buffer[32];
	snprintf(tag_buffer, sizeof(tag_buffer), "%04x", ctx->tagno);
	ctx->tag = tag_buffer;

	return ctx->tag;
}

int bio_read_bytes(struct context *ctx, char *buffer, int bytes) {
	while (bytes > 0) {
		logerr("[DEBUG] BIO_read(bytes=%d)\n", bytes);
		int bytes_read = BIO_read(ctx->bio, buffer, bytes);
		logerr("[DEBUG] BIO_read()=%d\n", bytes_read);
		if (bytes_read <= 0) {
			return -1;
		}

		buffer += bytes_read;
		bytes -= bytes_read;
	}
	return 0;
}

// handler for parsing command line arguments, using argp
error_t opt_handler(int key, char *arg, struct argp_state *state) {
	struct context *options = state->input;

	switch (key) {
		case 'u':
			options->username = arg;
			break;
		case 'p':
			options->password = arg;
			break;
		case 'f':
			options->folder = arg;
			break;
		case 'n':
			options->message_num = arg;
			break;
		case 't':
			options->use_tls = 1;
			break;
		case ARGP_KEY_ARG:
			if (state->arg_num == 0) {
				options->command = arg;
			} else if (state->arg_num == 1) {
				options->server_name = arg;
			} else {
				return ARGP_ERR_UNKNOWN;
			}
			break;
		case ARGP_KEY_END:
			if (state->arg_num < 2) {
				return ARGP_ERR_UNKNOWN;
			}
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

// parse command line arguments, and return as struct context
struct context *parse_cmdline_options(int argc, char *argv[]) {
	// supported options
	static struct argp_option const options[] = {
		{"username", 'u', "username", 0, "username to login"},
		{"password", 'p', "password", 0, "password to login"},
		{"folder", 'f', "folder", 0, "which folder you want to read from"},
		{"message-num", 'n', "message_num", 0, "message sequence number to fetch"},
		{"use-tls", 't', "use_tls", OPTION_ARG_OPTIONAL, "use tls"},
		{ 0 },
	};

	static struct context result = { 0 };

	static struct argp argp = { options, opt_handler, "command server_name", 0 };

	argp_parse(&argp, argc, argv, ARGP_NO_EXIT, 0, &result);

	return &result;
}

// connect to the imap server, using the command line arguments saved in context
// build ssl connection if -t is specified
// build BIO object for data read/write
int connect_imap_server_pro(struct context *ctx, int ai_family) {
	logerr("[DEBUG] connect_imap_server_pro(ctx=%p, ai_family=%d)", ctx, ai_family);

	// addrinfo for server name resolving
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = ai_family;
	hints.ai_socktype = SOCK_STREAM;

	// call getaddrinfo to resolve server name and service name
	// - if use tls, then service name is imaps, which resolved to 993
	// - otherwise, service name is imap, which resolved to 143
	struct addrinfo *result;
	int retval = getaddrinfo(ctx->server_name, ctx->use_tls ? "imaps" : "imap", &hints, &result);
	if (retval != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(retval));
		return -1;
	}

	// fd of client socket, in order to connect imap server
	int sfd = 0;

	// try resolved addresses one byone
	struct addrinfo *cursor;
	for (cursor = result; cursor != NULL; cursor = cursor->ai_next) {
		logerr("[DEBUG] socket\n");
		// create socket
		sfd = socket(cursor->ai_family, cursor->ai_socktype, cursor->ai_protocol);
		if (sfd == -1) {
			perror("socket");
			continue;
		}

		// connect to the server, address is resolved by getaddrinfo
		int retval = connect(sfd, cursor->ai_addr, cursor->ai_addrlen);
		logerr("[DEBUG] socket()=%d]n", retval);
		if (retval != -1) {
			break;
		} else {
			perror("connect");
		}

		close(sfd);
		sfd = retval;
	}

	// release memory returned by getaddrinfo
	freeaddrinfo(result);

	return sfd;
}

int connect_imap_server(struct context *ctx) {
	int sfd = connect_imap_server_pro(ctx, AF_INET6);
	if (sfd >= 0) {
		return sfd;
	}

	return connect_imap_server_pro(ctx, AF_INET);
}

int connect_and_login_imap_server(struct context *ctx) {
	int sfd = connect_imap_server(ctx);
	if (sfd < 0) {
		return ERR_CONNECTION;
	}

	ctx->sfd = sfd;

	if (ctx->use_tls) {
		// if use tls, call setup_tls to build tls connection
		if (setup_tls(ctx) != 0) {
			return ERR_CONNECTION;
		}
	} else {
		// otherwise, build BIO object for data read/write
		BIO *bio = BIO_new_fd(sfd, BIO_NOCLOSE);
		if (bio == NULL) {
			perror("BIO_new_fd");
			return ERR_CONNECTION;
		}

		ctx->bio = bio;
	}

	return login_imap_server(ctx);
}

// build stl connection over connected socket
int setup_tls(struct context *ctx) {
	logerr("[DEBUG] setup_tls\n");

	SSL_library_init();
	SSL_load_error_strings();
	OpenSSL_add_all_algorithms();

	logerr("[DEBUG] SSL_CTX_new\n");
	SSL_CTX *sslctx = SSL_CTX_new(TLS_method());
	if (sslctx == NULL) {
		perror("SSL_CTX_new");
		ERR_print_errors_fp(stderr);
		return 2;
	}

	ctx->sslctx = sslctx;

	// use default pathes for ca-cert
	logerr("[DEBUG] SSL_CTX_set_default_verify_paths\n");
	if (!SSL_CTX_set_default_verify_paths(sslctx)) {
		perror("SSL_CTX_set_default_verify_paths");
		ERR_print_errors_fp(stderr);
		return 2;
	}

	logerr("[DEBUG] SSL_new\n");
	SSL *ssl = SSL_new(sslctx);
	if (ssl == NULL) {
		perror("SSL_new");
		ERR_print_errors_fp(stderr);
		return 2;
	}

	logerr("[DEBUG] SSL_set_fd\n");
	if (!SSL_set_fd(ssl, ctx->sfd)) {
		perror("SSL_set_fd");
		ERR_print_errors_fp(stderr);
		return 2;
	}

	logerr("[DEBUG] SSL_connect\n");
	if (SSL_connect(ssl) != 1) {
		perror("SSL_connect");
		ERR_print_errors_fp(stderr);
		return 2;
	}

	logerr("[DEBUG] SSL_get_verify_result\n");
	if (SSL_get_verify_result(ssl) != X509_V_OK) {
		perror("SSL_get_verify_result");
		ERR_print_errors_fp(stderr);
		return 2;
	}

	logerr("[DEBUG] BIO_new_socket\n");
	BIO *bio = BIO_new_buffer_ssl_connect(sslctx);
	if (bio == NULL) {
		perror("BIO_new");
		ERR_print_errors_fp(stderr);
		return 2;
	}

	logerr("[DEBUG] BIO_set_ssl\n");
	int retval = BIO_set_ssl(bio, ssl, BIO_NOCLOSE);
	if (retval != 1) {
		perror("BIO_set_ssl");
		printf("%d\n", retval);
		ERR_print_errors_fp(stderr);
		return 2;
	}

	ctx->ssl = ssl;
	ctx->bio = bio;

	return 0;
};

// read online from server, using BIO object
char *read_line(struct context *ctx) {
	static char buffer[BUFFER_SIZE] = { 0 };
	memset(buffer, 0, BUFFER_SIZE);

	fprintf(stderr, "[DEBUG] BIO_gets(bio=%p, buffer=%p, size=%ld)\n", ctx->bio, buffer, sizeof(buffer));
	int bytes_read = BIO_gets(ctx->bio, buffer, sizeof(buffer));
	if (bytes_read <= 0) {
		return NULL;
	}
	fprintf(stderr, "[DEBUG] BIO_gets()=%d >>> %s\n", bytes_read, buffer);

	return buffer;
}

// read until result line starts with current tag
char *read_result_line(struct context *ctx) {
	fprintf(stderr, "[DEBUG] read_result_line()\n");

	for (;;) {
		char *line = read_line(ctx);
		if (line == NULL) {
			return NULL;
		}

		fprintf(stderr, "[DEBUG] read_line: %s\n", line);


		if (strncmp(line, ctx->tag, strlen(ctx->tag)) == 0) {
			return line;
		}
	}

	return NULL;
}

int send_command(struct context *ctx, const char *command, ...) {
	char buffer[BUFFER_SIZE] = { 0 };

	va_list args;
	va_start(args, command);

	int bytes_to_send = vsnprintf(buffer, sizeof(buffer), command, args);
	if (bytes_to_send <= 0) {
		return -1;
	}

	logerr("[DEBUG] BIO_write: [%d]%s\n", bytes_to_send, buffer);
	int bytes_sent = BIO_write(ctx->bio, buffer, bytes_to_send);
	logerr("[DEBUG] BIO_write: [%d]\n", bytes_sent);
	if (bytes_sent < 0) {
		return -1;
	} else if (bytes_sent != bytes_to_send) {
		return -1;
	}

	if (!BIO_flush(ctx->bio)) {
		return -1;
	}

	return 0;
}

int receive_message(struct context *ctx, char **body, char **linep) {
	char *message_num = NULL;
	char *body_bytes_raw = NULL;

	char *line = read_line(ctx);
	if (line == NULL) {
		return -1;
	}

	if (*line != '*') {
		if (linep != NULL) {
			*linep = line;
		}
		return 0;
	}

	char *end = line + strlen(line);

	// parse message num
	char *cursor = line + 1;
	for (;;) {
		if (cursor >= end) {
			return -1;
		}

		if (*cursor != ' ') {
			message_num = cursor;
			break;
		}

		cursor++;
	}

	for (;;) {
		if (cursor >= end) {
			return -1;
		}

		if (*cursor == ' ') {
			*cursor = '\0';
			break;
		}

		cursor++;
	}

	cursor = end - 1;

	for (;;) {
		if (cursor < line) {
			return -1;
		}

		if (*cursor == '}') {
			*cursor = '\0';
			break;
		}

		cursor--;
	}

	for (;;) {
		if (cursor < line) {
			return -1;
		}

		if (*cursor == '{') {
			body_bytes_raw = cursor + 1;
			break;
		}

		cursor--;
	}

	int body_bytes = atoi(body_bytes_raw);
	if (!body_bytes) {
		return -1;
	}

	logerr("[DEBUG] receive_message() message_num=%s body_bytes_raw=%s body_bytes=%d\n", message_num, body_bytes_raw, body_bytes);

	int buffer_size = body_bytes + 1 + strlen(message_num) + 1;
	char *buffer = malloc(buffer_size);
	logerr("[DEBUG] receive_message() buffer=%p\n", buffer);
	if (buffer == NULL) {
		return -1;
	}

	memset(buffer, 0, buffer_size);
	strcpy(buffer+body_bytes+1, message_num);

	if (bio_read_bytes(ctx, buffer, body_bytes) == -1) {
		safe_free(buffer);
		return -1;
	}

	if (read_line(ctx) == NULL) {
		safe_free(buffer);
		return -1;
	}

	*body = buffer;
	logerr("[DEBUG] receive_message() body=%p\n", *body);

	return body_bytes;
}

int login_imap_server(struct context *ctx) {
	char *line = read_line(ctx);
	if (line == NULL) {
		return ERR_IMAP;
	}

	logerr("[DEBUG] server greet: %s\n", line);

	char *password = dup_escape(ctx->password);
	if (password == NULL) {
		return ERR_IMAP;
	}

	if (send_command(ctx, "%s LOGIN %s %s\r\n\r\n", next_tag(ctx), ctx->username, password) != 0) {
		safe_free(password);
		return ERR_IMAP;
	}

	safe_free(password);

	line = read_result_line(ctx);
	fprintf(stderr, "[DEBUG] LOGIN result: %s\n", line);

	strlwr(line);
	if (strstr(line, "ok") == NULL) {
		printf("Login failure\n");
		return 3;
	}

	char *folder = ctx->folder;
	if (folder==NULL) {
		folder = "INBOX";
	} else if (strlen(folder)==0) {
		printf("specified folder is empty\n");
		return ERR_CMDLINE_ARGS;
	}

	if (send_command(ctx, "%s SELECT \"%s\"\r\n", next_tag(ctx), folder) != 0) {
		return ERR_IMAP;
	}

	line = read_result_line(ctx);
	fprintf(stderr, "[DEBUG] SELECT result: %s\n", line);

	strlwr(line);
	if (strstr(line, "ok") == NULL) {
		printf("Folder not found\n");
		return 3;
	}

	return 0;
}

int run_command_retrieve(struct context *ctx) {
	char *message_num = ctx->message_num;
	if (message_num == NULL) {
		message_num = "*";
	} else if (strstr(ctx->message_num, ":") != NULL) {
		printf("bad mesage num given\n");
		return ERR_CMDLINE_ARGS;
	} else if (strstr(ctx->message_num, ",") != NULL) {
		printf("bad mesage num given\n");
		return ERR_CMDLINE_ARGS;
	}

	if (send_command(ctx, "%s FETCH %s BODY.PEEK[]\r\n", next_tag(ctx), message_num) != 0) {
		return ERR_IMAP;
	}

	char *body = NULL;
	int body_bytes = receive_message(ctx, &body, NULL);
	if (body_bytes == -1) {
		safe_free(body);
		return ERR_IMAP;
	}

	if (body_bytes == 0) {
		safe_free(body);
		fprintf(stdout, "Message not found\n");
		return 3;
	}

	char *line = read_line(ctx);
	if (line == NULL) {
		safe_free(body);
		return ERR_IMAP;
	}

	if (write(1, body, body_bytes) != body_bytes) {
		safe_free(body);
		return ERR_IMAP;
	}

	safe_free(body);

	return 0;
}

int run_command_parse(struct context *ctx) {
	char *message_num = ctx->message_num;
	if (message_num == NULL) {
		message_num = "*";
	} else if (strstr(ctx->message_num, ":") != NULL) {
		printf("bad mesage num given\n");
		return ERR_CMDLINE_ARGS;
	} else if (strstr(ctx->message_num, ",") != NULL) {
		printf("bad mesage num given\n");
		return ERR_CMDLINE_ARGS;
	}

	if (send_command(ctx, "%s FETCH %s BODY.PEEK[HEADER.FIELDS (FROM TO DATE SUBJECT)]\r\n", next_tag(ctx), message_num) != 0) {
		return ERR_IMAP;
	}

	char *body = NULL;
	int body_bytes = receive_message(ctx, &body, NULL);
	if (body_bytes == -1) {
		safe_free(body);
		return ERR_IMAP;
	}
	logerr("content: %s\n", body);

	if (body_bytes == 0) {
		fprintf(stdout, "Message not found\n");
		safe_free(body);
		return 3;
	}

	char *line = read_line(ctx);
	if (line == NULL) {
		safe_free(body);
		return ERR_IMAP;
	}

	struct message message;
	memset(&message, 0, sizeof(struct message));

	if (parse_message(body, body_bytes, &message) == -1) {
		safe_free(body);
		return ERR_IMAP;
	}

	char *subject = message.subject;
	if (subject==NULL || strlen(subject)==0) {
		subject = "<No subject>";
	}

	char *to = message.to;

	printf("From: %s\n", message.from);
	if (to != NULL && strlen(to) > 0) {
		printf("To: %s\n", message.to);
	} else {
		printf("To:\n");
	}
	printf("Date: %s\n", message.date);
	printf("Subject: %s\n", subject);

	safe_free(body);

	return 0;
}

int run_command_mime(struct context *ctx) {
	char *message_num = ctx->message_num;
	if (message_num == NULL) {
		message_num = "*";
	} else if (strstr(ctx->message_num, ":") != NULL) {
		printf("bad mesage num given\n");
		return ERR_CMDLINE_ARGS;
	} else if (strstr(ctx->message_num, ",") != NULL) {
		printf("bad mesage num given\n");
		return ERR_CMDLINE_ARGS;
	}

	if (send_command(ctx, "%s FETCH %s BODY.PEEK[]\r\n", next_tag(ctx), message_num) != 0) {
		return ERR_IMAP;
	}

	char *body = NULL;
	int body_bytes = receive_message(ctx, &body, NULL);
	if (body_bytes == -1) {
		safe_free(body);
		return ERR_IMAP;
	}

	if (body_bytes == 0) {
		safe_free(body);
		fprintf(stdout, "Message not found\n");
		return 3;
	}

	write(2, body, body_bytes/2);

	char *line = read_line(ctx);
	if (line == NULL) {
		safe_free(body);
		return ERR_IMAP;
	}

	struct message message;
	memset(&message, 0, sizeof(struct message));

	if (parse_message(body, body_bytes, &message) == -1) {
		safe_free(body);
		return ERR_IMAP;
	}

	char *content_type = message.content_type;
	if (content_type == NULL) {
		safe_free(body);
		printf("no content type header\n");
		return 4;
	}

	logerr("[DEBUG] run_command_mime() content_type=%s\n", content_type);

	if (strstr(content_type, "multipart/alternative")==NULL) {
		safe_free(body);
		printf("content type is not multipart/alternative\n");
		return 4;
	}

	char *content_type2 = strdup(content_type);
	if (content_type2 == NULL) {
		safe_free(body);
		return 4;
	}

	strlwr(content_type2);
	logerr("[DEBUG] run_command_mime() content_type2=%s\n", content_type2);

	char *boundary = strstr(content_type2, "boundary");
	if (boundary == NULL) {
		safe_free(content_type2);
		safe_free(body);
		printf("no boundary parameter\n");
		return 4;
	}

	boundary = content_type + (boundary-content_type2);
	safe_free(content_type2);

	char *boundary_value = strstr(boundary, "\"");
	if (boundary_value == NULL) {
		boundary_value = strstr(boundary, "=");
		if (boundary_value == NULL) {
			safe_free(body);
			printf("bad boundary parameter\n");
			return 4;
		}
	}
	boundary = boundary_value + 1;

	char *boundary_end = strstr(boundary, "\"");
	if (boundary_end == NULL) {
		// safe_free(body);
		// printf("bad boundary parameter\n");
		// return 4;
	} else {
		*boundary_end = '\0';
	}

	int boundary_len = strlen(boundary);

	logerr("[DEBUG] run_command_mime() boundary=\"%s\" boundary_len=%d\n", boundary, boundary_len);

	char *body_end = message.body + message.body_bytes;

	char *part_start = strstr(message.body, boundary);
	if (part_start == NULL) {
		printf("no boundary in body\n");
		safe_free(body);
		return 4;
	}

	part_start += boundary_len + 2;
	char *part_end;
	for (; part_start < body_end; part_start = part_end + boundary_len + 6) {
		logerr("part_start=%d/%d\n", part_start-message.body, message.body_bytes);

		part_end = strstr(part_start, boundary);
		if (part_end == NULL) {
			printf("no boundary end\n");
			safe_free(body);
			return 4;
		}

		part_end -= 4;

		int part_bytes = part_end - part_start;
		logerr("part_found: start=%p bytes=%d\n", part_start, part_bytes);

		struct message part;
		memset(&part, 0, sizeof(part));

		if (parse_message(part_start, part_bytes, &part) == -1) {
			logerr("parse part error\n");
			continue;
		}

		char *content_type = part.content_type;
		if (content_type == NULL) {
			logerr("not content type in part\n");
			continue;
		}

		strlwr(content_type);
		logerr("part content-type: \"%s\"\n", content_type);

		if (strstr(content_type, "text/plain") != NULL && strstr(content_type, "utf-8") != NULL) {
			safe_free(body);
			write(1, part.body, part.body_bytes);
			return 0;
		}
	}

	safe_free(body);

	return 4;
}

int run_command_list(struct context *ctx) {
	if (send_command(ctx, "%s FETCH 1:* (FLAGS BODY[HEADER.FIELDS (SUBJECT)])\r\n", next_tag(ctx), ctx->username, ctx->password) != 0) {
		return ERR_IMAP;
	}

	for (;;) {
		char *body = NULL;
		int body_bytes = receive_message(ctx, &body, NULL);
		if (body_bytes == -1) {
			safe_free(body);
			return ERR_IMAP;
		}

		if (body_bytes == 0) {
			safe_free(body);
			break;
		}

		char *message_num = body + body_bytes + 1;

		struct message message;
		memset(&message, 0, sizeof(message));
		if (parse_message(body, body_bytes, &message) == -1) {
			safe_free(body);
			return ERR_IMAP;
		}

		char *subject = message.subject;
		if (subject == NULL || strlen(subject) == 0) {
			subject = "<No subject>";
		}

		printf("%s: %s\n", message_num, subject);

		safe_free(body);
	}

	return 0;
}

int run_command(struct context *ctx) {
	if (strcmp(ctx->command, "retrieve") == 0) {
		return run_command_retrieve(ctx);
	} else if (strcmp(ctx->command, "parse") == 0) {
		return run_command_parse(ctx);
	} else if (strcmp(ctx->command, "mime") == 0) {
		return run_command_mime(ctx);
	} else if (strcmp(ctx->command, "list") == 0) {
		return run_command_list(ctx);
	} else {
		printf("Unknown command: %s\n", ctx->command);
		return ERR_CMDLINE_ARGS;
	}
}

int main(int argc, char *argv[]) {
	struct context *ctx = parse_cmdline_options(argc, argv);
	if (ctx == NULL) {
		return ERR_CMDLINE_ARGS;
	}

	if (strempty(ctx->username)) {
		printf("Username not specified\n");
		return ERR_CMDLINE_ARGS;
	}

	if (strempty(ctx->password)) {
		printf("Password not specified\n");
		return ERR_CMDLINE_ARGS;
	}

	if (strempty(ctx->command)) {
		printf("Command not specified\n");
		return ERR_CMDLINE_ARGS;
	}

	if (strempty(ctx->server_name)) {
		printf("Server name not specified\n");
		return ERR_CMDLINE_ARGS;
	}

	int retval = connect_and_login_imap_server(ctx);
	if (retval != 0) {
		perror("connect imap server failed");
		goto free;
	}

	retval = run_command(ctx);
	if (retval != 0) {
		send_command(ctx, "LOGOUT\r\n");
	}

free:
	if (ctx->bio != NULL) {
		BIO_free_all(ctx->bio);
	}
	if (ctx->ssl != NULL) {
		SSL_shutdown(ctx->ssl);
		SSL_free(ctx->ssl);
	}
	if (ctx->sslctx != NULL) {
		SSL_CTX_free(ctx->sslctx);
	}
	if (ctx->sfd >= 0) {
		close(ctx->sfd);
	}

	return retval;
}
