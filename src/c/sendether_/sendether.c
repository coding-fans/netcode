/*
 * Author: fasion
 * Created time: 2020-10-27 19:52:25
 * Last Modified by: fasion
 * Last Modified time: 2021-02-18 15:38:34
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

#define ETHERNET_HEADER_SIZE 14
#define MAX_ETHERNET_DATA_SIZE 1500


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
    return 0;
}


/**
 * struct for an ethernet frame
 **/
struct __attribute__((__packed__)) ethernet_frame {

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
    return 0;
}


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
int send_ether_frame(int s, const unsigned char *fr, const unsigned char *to,
        short type, const char *data) {
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

    return 0;
}
