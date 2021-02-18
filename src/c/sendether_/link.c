/*
 * Author: fasion
 * Created time: 2021-01-13 15:45:07
 * Last Modified by: fasion
 * Last Modified time: 2021-02-18 15:21:15
 */

#include <net/if.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>

#include "link.h"


/**
 *  Convert readable MAC address to binary format.
 *
 *  Arguments
 *      a: buffer for readable format, like "08:00:27:c8:04:83".
 *
 *      n: buffer for binary format, 6 bytes at least.
 *
 *  Returns
 *      0 if success, -1 if error.
 **/
int mac_aton(const char *a, unsigned char *n) {
    int matches = sscanf(a, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", n, n+1, n+2,
                         n+3, n+4, n+5);

    return (6 == matches ? 0 : -1);
}


/**
 *  Fetch MAC address of given iface.
 *
 *  Arguments
 *      s: socket for ioctl, optional.
 *
 *      iface: name of given iface.
 *
 *      mac: buffer for binary MAC address, 6 bytes at least.
 *
 *  Returns
 *      0 if success, -1 if error.
 **/
int fetch_iface_mac(int s, const char *iface, unsigned char *mac) {
    // fill iface name to struct ifreq
    struct ifreq ifr;
    strncpy(ifr.ifr_name, iface, 15);

    // call ioctl to get hardware address
    if (ioctl(s, SIOCGIFHWADDR, &ifr) == -1) {
        return -1;
    }

    // copy MAC address to given buffer
    memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);

    return 0;
}

/**
 *  Fetch index of given iface.
 *
 *  Arguments
 *      iface: name of given iface.
 *
 *      s: socket for ioctl, optional.
 *
 *  Returns
 *      Iface index(which is greater than 0) if success, -1 if error.
 **/
int fetch_iface_index(int s, const char *iface) {
    // fill iface name to struct ifreq
    struct ifreq ifr;
    strncpy(ifr.ifr_name, iface, 15);

    // call ioctl system call to fetch iface index
    if (ioctl(s, SIOCGIFINDEX, &ifr) == -1) {
        return -1;
    }

    return ifr.ifr_ifindex;
}
