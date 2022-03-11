/*
 * Author: fasion
 * Created time: 2021-02-23 19:35:34
 * Last Modified by: fasion
 * Last Modified time: 2022-02-28 09:48:43
 */

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "argparse.h"
#include "common.h"

int main(int argc, char *argv[]) {
    // parse cmdline arguments
    const struct client_cmdline_arguments *arguments = parse_client_arguments(argc, argv);
    if (arguments == NULL) {
        fprintf(stderr, "Failed to parse cmdline arguments\n");
        return -1;
    }

    // check time format string length
    if (strlen(arguments->time_format) + 1 > MAX_FORMAT_SIZE) {
        fprintf(stderr, "Time format is to long\n");
        return -1;
    }

    // create socket for udp communication
    int s = socket(PF_INET, SOCK_DGRAM, 0);
    if (s == -1) {
        perror("Failed to create socket");
        return -1;
    }

    // struct for storing server address
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(arguments->server_port);

    // parse ip address
    if (inet_aton(arguments->server_ip, &server_addr.sin_addr) == 0) {
        fprintf(stderr, "Invalid IP: %s\n", arguments->server_ip);
        return -1;
    }

    // build request message
    struct time_request request;
    bzero(&request);

    int format_bytes = stpncpy(request.format, arguments->time_format, MAX_FORMAT_SIZE-1) - request.format + 1;
    request.bytes = htonl(format_bytes);
    int request_len = sizeof(request.bytes) + format_bytes;

    // send request
    if (sendto(s, &request, request_len, 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Failed to send request");
        return -1;
    }

    // receive reply
    struct time_reply reply;
    if (recvfrom(s, &reply, sizeof(reply), 0, NULL, NULL) == -1) {
        perror("Failed to receive reply");
        return -1;
    }

    // print reply
    printf("Receive %d bytes\n", ntohl(reply.bytes));

    // print time data
    if (reply.bytes > 0) {
        reply.time[MAX_DATA_SIZE-1] = '\0';
        printf("%s\n", reply.time);
    }

    return 0;
}
