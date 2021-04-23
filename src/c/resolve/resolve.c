/*
 * Author: fasion
 * Created time: 2021-04-19 10:42:59
 * Last Modified by: fasion
 * Last Modified time: 2021-04-19 13:10:07
 */

#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "bad arguments");
        return -1;
    }

    char *name = argv[1];
    printf("resolve domain name: %s\n", name);

    struct hostent *result = gethostbyname(name);
    if (result == NULL) {
        if (h_errno == HOST_NOT_FOUND) {
            fprintf(stderr, "Hostname not found!\n");
        }

        if (h_errno == NO_DATA) {
            fprintf(stderr, "No such record\n");
        }

        if (h_errno == NO_RECOVERY) {
            fprintf(stderr, "\n");
        }

        if (h_errno == TRY_AGAIN) {
            fprintf(stderr, "Temporary error occurred, please try again!\n");
        }

        return -1;
    }

    int i = 0;
    while (result->h_addr_list[i] != NULL) {
        printf("IP: %s\n", inet_ntoa(*(struct in_addr *)result->h_addr_list[i]));
        i++;
    }

    return 0;
}
