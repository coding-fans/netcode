/*
 * Author: fasion
 * Created time: 2021-02-23 19:35:34
 * Last Modified by: fasion
 * Last Modified time: 2021-02-24 16:52:02
 */

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "argparse.h"
#include "common.h"

int main(int argc, char *argv[]) {
    const struct client_cmdline_arguments *arguments = parse_client_arguments(argc, argv);
    if (arguments == NULL) {
        fprintf(stderr, "Failed to parse cmdline arguments\n");
        return -1;
    }

    if (strlen(arguments->time_format) + 1 > MAX_FORMAT_SIZE) {
        fprintf(stderr, "Time format is to long\n");
        return -1;
    }

    int s = socket(PF_INET, SOCK_DGRAM, 0);
    if (s == -1) {
        perror("Failed to create socket");
        return -1;
    }

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));

    server_addr.sin_family = PF_INET;
    server_addr.sin_port = htons(arguments->server_port);

    if (inet_aton(arguments->server_ip, &server_addr.sin_addr) == 0) {
        fprintf(stderr, "Invalid IP: %s\n", arguments->server_ip);
        return -1;

    }

    struct time_request request;
    request.bytes = strlen(arguments->time_format) + 1;
    strncpy(request.format, arguments->time_format, MAX_FORMAT_SIZE);

    if (sendto(s, &request, request.bytes + sizeof(request.bytes), 0,
            (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Failed to send request");
        return -1;
    }

    struct time_reply reply;
    if (recvfrom(s, &reply, sizeof(reply), 0, NULL, NULL) == -1) {
        perror("Failed to receive reply");
        return -1;
    }

    printf("Receive %d bytes\n", reply.bytes);

    reply.time[MAX_DATA_SIZE-1] = '\0';
    printf("%s\n", reply.time);

    return 0;
}
