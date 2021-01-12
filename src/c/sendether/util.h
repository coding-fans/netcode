/*
 * Author: fasion
 * Created time: 2020-10-27 20:28:34
 * Last Modified by: fasion
 * Last Modified time: 2020-10-28 10:33:45
 */

/**
 * struct for storing command line arguments.
 **/
struct cmdline_arguments {
    // name of iface through which data is sent
    char const *iface;

    // destination MAC address
    char const *to;

    // data type
    unsigned short type;

    // data to send
    char const *data;
};

int mac_aton(const char *a, unsigned char *n);
struct cmdline_arguments const *parse_arguments(int argc, char *argv[]);
