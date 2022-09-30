/*
 * Author: fasion
 * Created time: 2021-05-13 18:35:10
 * Last Modified by: fasion
 * Last Modified time: 2022-09-28 14:35:43
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

    // fill address family & server port
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(arguments->server_port);

    // fill server ip address
    if (inet_aton(arguments->server_ip, &server_addr.sin_addr) != 1) {
        perror("Bad server address");
        return -1;
    }

    // connect to server
    if (connect(s, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Failed to connect server");
        return -1;
    }

    // main loop
    for (;;) {
        printf("> ");

        // read user input
        char line[MAX_LINE_LEN];
        if (fgets(line, MAX_LINE_LEN, stdin) == NULL) {
            printf("\nbye.\n");
            break;
        }

        if (strlen(line) == 0) {
            continue;
        }

        // send it to server
        if (send(s, line, strlen(line), 0) == -1) {
            perror("Failed to send data");
            return -1;
        }

        // receive response from server
        int bytes = recv(s, line, MAX_LINE_LEN, 0);
        if (bytes == -1) {
            perror("Failed to receive data");
            return -1;
        }

        // write responsed data to stdout
        if (write(STDOUT_FILENO, line, bytes) == -1) {
            perror("Failed to write stdout");
            return -1;
        }
    }

    return 0;
}
