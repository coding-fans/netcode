/*
 * Author: fasion
 * Created time: 2020-10-27 19:52:25
 * Last Modified by: fasion
 * Last Modified time: 2021-01-12 17:11:17
 */

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include "util.h"

#define MAX_ETHERNET_FRAME_SIZE 1514
#define MAX_ETHERNET_DATA_SIZE 1500

#define ETHERNET_HEADER_SIZE 14
#define ETHERNET_DST_ADDR_OFFSET 0
#define ETHERNET_SRC_ADDR_OFFSET 6
#define ETHERNET_TYPE_OFFSET 12
#define ETHERNET_DATA_OFFSET 14

#define MAC_BYTES 6


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
int fetch_iface_mac(int s, char const *iface, unsigned char *mac) {
    // fill iface name to struct ifreq
    struct ifreq ifr;
    strncpy(ifr.ifr_name, iface, 15);

    // call ioctl to get hardware address
    if (ioctl(s, SIOCGIFHWADDR, &ifr) == -1) {
        return -1;
    }

    // copy MAC address to given buffer
    memcpy(mac, ifr.ifr_hwaddr.sa_data, MAC_BYTES);

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
int fetch_iface_index(int s, char const *iface) {
    // fill iface name to struct ifreq
    struct ifreq ifr;
    strncpy(ifr.ifr_name, iface, 15);

    // call ioctl system call to fetch iface index
    if (ioctl(s, SIOCGIFINDEX, &ifr) == -1 ) {
        return -1;
    }

    return ifr.ifr_ifindex;
}


/**
 * Bind socket with given iface.
 *
 *  Arguments
 *      s: given socket.
 *
 *      iface: name of given iface.
 *
 *  Returns
 *      0 if success, -1 if error.
 **/
int bind_iface(int s, char const *iface) {
    // fetch iface index
    int if_index = fetch_iface_index(s, iface);
    if (if_index == -1) {
        return -1;
    }

    // fill iface index to struct sockaddr_ll for binding
    struct sockaddr_ll sll;
    bzero(&sll, sizeof(sll));
    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = if_index;
    sll.sll_pkttype = PACKET_HOST;

    // call bind system call to bind socket with iface
    if(bind(s, (struct sockaddr *)&sll, sizeof(sll)) == -1) {
        return -1;
    }

    return 0;
}


/**
 * struct for an ethernet frame
 **/
struct ethernet_frame {
    // destination MAC address, 6 bytes
    unsigned char dst_addr[6];

    // source MAC address, 6 bytes
    unsigned char src_addr[6];

    // type, in network byte order
    unsigned short type;

    // data
    unsigned char data[MAX_ETHERNET_DATA_SIZE];
};


/**
 *  Send data through given iface by ethernet protocol, using raw socket.
 *
 *  Arguments
 *      iface: name of iface for sending.
 *
 *      to: destination MAC address, in binary format.
 *
 *      type: protocol type.
 *
 *      data: data to send, ends with '\0'.
 *
 *      s: socket for ioctl, optional.
 *
 *  Returns
 *      0 if success, -1 if error.
 **/
int send_ether(int s, unsigned char const *fr, unsigned char const *to,
        short type, char const *data) {
    // construct ethernet frame, which can be 1514 bytes at most
    struct ethernet_frame frame;

    // fill destination MAC address
    memcpy(frame.dst_addr, to, MAC_BYTES);

    // fill source MAC address
    memcpy(frame.src_addr, fr, MAC_BYTES);

    // fill type
    frame.type = htons(type);

    // truncate if data is to long
    int data_size = strlen(data);
    if (data_size > MAX_ETHERNET_DATA_SIZE) {
        data_size = MAX_ETHERNET_DATA_SIZE;
    }

    // fill data
    memcpy(frame.data, data, data_size);

    int frame_size = ETHERNET_HEADER_SIZE + data_size;

    if (sendto(s, &frame, frame_size, 0, NULL, 0) == -1) {
        return -1;
    }

    return 0;
}


int main(int argc, char *argv[]) {
    // parse command line options to struct arguments
    struct cmdline_arguments const *arguments = parse_arguments(argc, argv);
    if (arguments == NULL) {
        fprintf(stderr, "Bad command line options given\n");
        return -1;
    }

    // check arguments
    if (arguments->iface == NULL) {
        fprintf(stderr, "No iface given\n");
        return -1;
    }
    if (arguments->to == NULL) {
        fprintf(stderr, "No destination mac address given\n");
        return -1;
    }
    if (strlen(arguments->to) < 17) {
        fprintf(stderr, "Bad destination mac address given %s\n", arguments->to);
        return -1;
    }

    // convert destinaction MAC address to binary format
    unsigned char to[6];
    if (mac_aton(arguments->to, to) != 0) {
        fprintf(stderr, "Bad destination mac address given %s\n", arguments->to);
        return -1;
    }

    // create socket for communication
    int s = socket(PF_PACKET, SOCK_RAW | SOCK_CLOEXEC, 0);
    if (-1 == s) {
        perror("Fail to create socket: ");
        return -1;
    }

    // bind socket with iface
    if (bind_iface(s, arguments->iface) == -1) {
        perror("Fail to bind socket with iface: ");
        return -1;
    }

    // fetch MAC address of given iface, which is the source address
    unsigned char fr[6];
    if (fetch_iface_mac(s, arguments->iface, fr) == -1) {
        perror("Fail to fetch mac of iface: ");
        return -1;
    }

    // send data
    if (send_ether(s, fr, to, arguments->type, arguments->data) == -1) {
        perror("Fail to send ethernet frame: ");
        return -1;
    }

    return 0;
}
