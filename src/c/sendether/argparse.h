/*
 * Author: fasion
 * Created time: 2020-10-27 20:28:34
 * Last Modified by: fasion
 * Last Modified time: 2021-01-13 17:13:59
 */

/**
 * struct for storing command line arguments.
 **/
struct cmdline_arguments {
    // name of iface through which data is sent
    const char *iface;

    // destination MAC address
    const char *to;

    // data type
    unsigned short type;

    // data to send
    const char *data;
};

const struct cmdline_arguments *parse_arguments(int argc, char *argv[]);
