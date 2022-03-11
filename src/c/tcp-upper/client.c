/*
 * Author: fasion
 * Created time: 2021-05-13 18:35:10
 * Last Modified by: fasion
 * Last Modified time: 2022-02-28 09:22:47
 */

#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>

#include "argparse.h"

#define MAX_LINE_LEN 10240

int main(int argc, char *argv[]) {
    // parse cmdline arguments
    const struct client_cmdline_arguments *arguments = parse_client_arguments(argc, argv);
    if (arguments == NULL) {
        fprintf(stderr, "Failed to parse cmdline arguments\n");
        return -1;
    }

    // check time format string length
    int s = socket(PF_INET, SOCK_STREAM, 0);
    if (s == -1) {
        perror("Failed to create socket");
        return -1;
    }

    // server address
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(arguments->server_port);

    if (inet_aton(arguments->server_ip, &server_addr.sin_addr) != 1) {
        perror("Bad server address");
        return -1;
    }

    if (connect(s, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Failed to connect server");
        return -1;
    }

    for (;;) {
        printf("> ");

        char line[MAX_LINE_LEN];
        if (fgets(line, MAX_LINE_LEN, stdin) == NULL) {
            printf("\nbye.\n");
            break;
        }

        if (strlen(line) == 0) {
            continue;
        }

        if (send(s, line, strlen(line), 0) == -1) {
            perror("Failed to send data");
            return -1;
        }

        int bytes = recv(s, line, MAX_LINE_LEN, 0);
        if (bytes == -1) {
            perror("Failed to receive data");
            return -1;
        }

        if (write(STDOUT_FILENO, line, bytes) == -1) {
            perror("Failed to write stdout");
            return -1;
        }
    }

	return 0;
}
