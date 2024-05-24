/*
 * Author: fasion
 * Created time: 2021-05-13 18:35:36
 * Last Modified by: fasion
 * Last Modified time: 2022-10-13 17:48:18
 */

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>

#include "argparse.h"

#define BUFFER_SIZE 102400

void uppercase(void *input, int bytes) {
    char *buffer = (char *)input;

    while (bytes > 0) {
        bytes--;
        buffer[bytes] = toupper(buffer[bytes]);
    }
}

void send_data(int s, void *data, int bytes) {
    while (bytes > 0) {
        int sent = send(s, data, bytes, 0);
        if (sent == -1) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }

            perror("Failed to send");
            break;
        }

        data += sent;
        bytes -= sent;
    }
}

void process_connection(int s) {
    for (;;) {
        char input[BUFFER_SIZE];

        int bytes = recv(s, input, sizeof(input), 0);
        if (bytes == -1) {
            if (errno == EINTR) {
                continue;
            }

            perror("Failed to recv");
            break;
        }

        if (bytes == 0) {
            break;
        }

        uppercase(input, bytes);

        send_data(s, input, bytes);
    }
}

int main(int argc, char *argv[]) {
    // parse cmdline arguments
    const struct server_cmdline_arguments *arguments = parse_server_arguments(argc, argv);
    if (arguments == NULL) {
        fprintf(stderr, "Failed to parse cmdline arguments\n");
        return -1;
    }

    // create socket
    int s = socket(PF_INET, SOCK_STREAM, 0);
    if (s == -1) {
        perror("Failed to create socket");
        return -1;
    }

    // server bind address
    struct sockaddr_in bind_addr;
    bzero(&bind_addr, sizeof(bind_addr));

    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = INADDR_ANY;
    bind_addr.sin_port = htons(arguments->port);

    // parse ip address
    if (arguments->bind_ip != NULL) {
        if (inet_aton(arguments->bind_ip, &bind_addr.sin_addr) == 0) {
            fprintf(stderr, "Invalid IP: %s\n", arguments->bind_ip);
            return -1;
        }
    }

    // bind socket with given port
    if (bind(s, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) == -1) {
        perror("Failed to bind address");
        close(s);
        return -1;
    }

    // listen for new connections
    if (listen(s, 100) == -1) {
        perror("Failed to listen");
        close(s);
        return -1;
    }

    printf("listening at port: %s:%d, waiting for connections...\n", arguments->bind_ip, arguments->port);

    for (;;) {
        // buffer for storing peer address
        struct sockaddr_in peer_addr;
        int addr_len = sizeof(peer_addr);

        // accept one connection
        int conn = accept(s, (struct sockaddr *)&peer_addr, &addr_len);
        if (conn == -1) {
            if (errno == EINTR) {
                continue;
            }

            perror("Failed to accept");
            break;
        }

        printf("\n%s:%d connected\n", inet_ntoa(peer_addr.sin_addr), ntohs(peer_addr.sin_port));

        // process for this connection
        process_connection(conn);

        // close when disconnected
        close(conn);

        printf("%s:%d disconnected\n", inet_ntoa(peer_addr.sin_addr), ntohs(peer_addr.sin_port));
    }

    // close listen socket
    close(s);

    return 0;
}
