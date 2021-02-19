/*
 * Author: fasion
 * Created time: 2020-10-27 19:52:25
 * Last Modified by: fasion
 * Last Modified time: 2021-02-18 15:22:27
 */

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include "argparse.h"
#include "link.h"

#define FRAME_HEADER_SIZE 14
#define MAX_FRAME_DATA_SIZE 1500


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
int bind_iface(int s, const char *iface) {
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
struct __attribute__((__packed__)) ethernet_frame {
    // destination MAC address, 6 bytes
    unsigned char dst_addr[6];

    // source MAC address, 6 bytes
    unsigned char src_addr[6];

    // type, in network byte order
    unsigned short type;

    // data
    unsigned char data[MAX_FRAME_DATA_SIZE];
};


/**
 * Pack ethernet frame
 *
 *  Arguments:
 *      fr: source mac address
 *
 *      to: destination mac address
 *
 *      type: ether type
 *
 *      data: data for sending
 *
 *      data_length: length of data
 *
 *      frame: frame to pack
 *
 *  Returns:
 *      Frame size
 **/
int pack_ether_frame(const unsigned char *fr, const unsigned char *to, short type,
        const char *data, int data_length, struct ethernet_frame *frame) {
    // fill destination MAC address
    memcpy(frame->dst_addr, to, 6);

    // fill source MAC address
    memcpy(frame->src_addr, fr, 6);

    // fill type
    frame->type = htons(type);

    // truncate if data is to long
    if (data_length > MAX_FRAME_DATA_SIZE) {
        data_length = MAX_FRAME_DATA_SIZE;
    }

    // fill data
    memcpy(frame->data, data, data_length);

    return FRAME_HEADER_SIZE + data_length;
}


/**
 *  Send data through given iface by ethernet protocol, using raw socket.
 *
 *  Arguments
 *      s: socket for sending.
 *
 *      iface: name of iface for sending.
 *
 *      to: destination MAC address, in binary format.
 *
 *      type: protocol type.
 *
 *      data: data to send, ends with '\0'.
 *
 *  Returns
 *      0 if success, -1 if error.
 **/
int send_ether_frame(int s, const unsigned char *fr, const unsigned char *to,
        short type, const char *data) {
    // construct ethernet frame, which can be 1514 bytes at most
    struct ethernet_frame frame;

    // pack frame
    int frame_size = pack_ether_frame(fr, to, type, data, strlen(data), &frame);

    // send the frame
    if (sendto(s, &frame, frame_size, 0, NULL, 0) == -1) {
        return -1;
    }

    return 0;
}


int main(int argc, char *argv[]) {
    // parse command line options to struct arguments
    const struct cmdline_arguments *arguments = parse_arguments(argc, argv);
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
    if (s == -1) {
        perror("Fail to create socket");
        return -1;
    }

    // fetch MAC address of given iface, which is the source address
    unsigned char fr[6];
    if (fetch_iface_mac(s, arguments->iface, fr) == -1) {
        perror("Fail to fetch mac of iface");
        return -1;
    }

    // bind socket with iface
    if (bind_iface(s, arguments->iface) == -1) {
        perror("Fail to bind socket with iface");
        return -1;
    }

    // send data
    if (send_ether_frame(s, fr, to, arguments->type, arguments->data) == -1) {
        perror("Fail to send ethernet frame");
        return -1;
    }

    return 0;
}
