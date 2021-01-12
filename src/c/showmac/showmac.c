/*
 * Author: fasion
 * Created time: 2020-10-27 19:40:17
 * Last Modified by: fasion
 * Last Modified time: 2020-10-30 10:52:08
 */

#include <net/if.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

/**
 *  Convert binary MAC address to readable format.
 *
 *  Arguments
 *      n: binary format, must be 6 bytes.
 *
 *      a: buffer for readable format, 18 bytes at least(`\0` included).
 **/
void mac_ntoa(unsigned char *n, char *a) {
    // traverse 6 bytes one by one
    sprintf(a, "%02x:%02x:%02x:%02x:%02x:%02x", n[0], n[1], n[2], n[3], n[4], n[5]);
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "no iface given\n");
        return 1;
    }

    // create a socket, any type is ok
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == s) {
        perror("Fail to create socket");
        return 2;
    }

    // fill iface name to struct ifreq
    struct ifreq ifr;
    strncpy(ifr.ifr_name, argv[1], 15);

    // call ioctl to get hardware address
    int ret = ioctl(s, SIOCGIFHWADDR, &ifr);
    if (-1 == ret) {
        perror("Fail to get mac address");
        return 3;
    }

    // convert to readable format
    char mac[18];
    mac_ntoa((unsigned char *)ifr.ifr_hwaddr.sa_data, mac);

    // output result
    printf("IFace: %s\n", ifr.ifr_name);
    printf("MAC: %s\n", mac);

    return 0;
}
