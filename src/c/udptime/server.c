/*
 * Author: fasion
 * Created time: 2021-02-23 16:11:52
 * Last Modified by: fasion
 * Last Modified time: 2021-02-24 16:52:23
 */

#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <time.h>

#include "argparse.h"
#include "common.h"

int main(int argc, char *argv[]) {
    const struct server_cmdline_arguments *arguments = parse_server_arguments(argc, argv);
    if (arguments == NULL) {
        fprintf(stderr, "Failed to parse cmdline arguments\n");
        return -1;
    }

    int s = socket(PF_INET, SOCK_DGRAM, 0);
    if (s == -1) {
        perror("Failed to create socket");
        return -1;
    }

    struct sockaddr_in bind_addr;
    bzero(&bind_addr, sizeof(bind_addr));

    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = INADDR_ANY;
    bind_addr.sin_port = htons(arguments->port);

    if (bind(s, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) == -1) {
        perror("Failed to bind address");
        return -1;
    }

    for (;;) {
        struct time_request request;

        struct sockaddr_in peer_addr;
        int addr_len = sizeof(peer_addr);

        int bytes = recvfrom(s, &request, sizeof(request), 0, (struct sockaddr *)&peer_addr, &addr_len);
        if (bytes == -1) {
            if (errno == EINTR) {
                continue;
            }

            perror("Failed to receive data");

            return -1;
        }

        request.format[MAX_FORMAT_SIZE - 1] = '\0';
        printf("%s:%d request with format: %s\n", inet_ntoa(peer_addr.sin_addr), ntohs(peer_addr.sin_port), request.format);

        struct time_reply reply;

        time_t now;
        time(&now);

        struct tm *local_time = localtime(&now);
        if (local_time == NULL) {
            perror("fetch local time");
            return -1;
        }

        size_t data_bytes = strftime(reply.time, MAX_DATA_SIZE, request.format, local_time);
        reply.bytes = data_bytes;

        if (sendto(s, &reply, bytes+4, 0, (struct sockaddr *)&peer_addr, sizeof(peer_addr)) == -1) {
            perror("Failed to send");
            return -1;
        }
    }

    return 0;
}
