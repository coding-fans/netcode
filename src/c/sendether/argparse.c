/*
 * Author: fasion
 * Created time: 2020-10-27 20:22:53
 * Last Modified by: fasion
 * Last Modified time: 2021-01-13 17:10:10
 */

#include <argp.h>

#include "argparse.h"


/**
 * opt_handler function for GNU argp.
 **/
static error_t opt_handler(int key, char *arg, struct argp_state *state) {
    struct cmdline_arguments *arguments = state->input;

    switch(key) {
        case 'd':
            arguments->data = arg;
            break;

        case 'i':
            arguments->iface = arg;
            break;

        case 'T':
            if (sscanf(arg, "%hx", &arguments->type) != 1) {
                return ARGP_ERR_UNKNOWN;
            }
            break;

        case 't':
            arguments->to = arg;
            break;

        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}


/**
 * Parse command line arguments given by argc, argv.
 *
 *  Arguments
 *      argc: the same with main function.
 *
 *      argv: the same with main function.
 *
 *  Returns
 *      Pointer to struct arguments if success, NULL if error.
 **/
const struct cmdline_arguments *parse_arguments(int argc, char *argv[]) {
    // docs for program and options
    static char const doc[] = "send_ether: send data through ethernet frame";
    static char const args_doc[] = "";

    // command line options
    static struct argp_option const options[] = {
        // Option -i --iface: name of iface through which data is sent
        {"iface", 'i', "IFACE", 0, "name of iface for sending"},

        // Option -t --to: destination MAC address
        {"to", 't', "TO", 0, "destination mac address"},

        // Option -T --type: data type
        {"type", 'T', "TYPE", 0, "data type"},

        // Option -d --data: data to send, optional since default value is set
        {"data", 'd', "DATA", 0, "data to send"},

        { 0 }
    };

    static const struct argp argp = {
        options,
        opt_handler,
        args_doc,
        doc,
        0,
        0,
        0,
    };

    // for storing results
    static struct cmdline_arguments arguments = {
        .iface = NULL,
        .to = NULL,
        //default data type: 0x0900
        .type = 0x0900,
        // default data, 46 bytes string of 'a'
        // since for ethernet frame data is 46 bytes at least
        .data = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
    };

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    return &arguments;
}
