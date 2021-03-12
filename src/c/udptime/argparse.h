/*
 * Author: fasion
 * Created time: 2020-10-27 20:28:34
 * Last Modified by: fasion
 * Last Modified time: 2021-02-24 19:05:07
 */

/*
 * struct for storing server command line arguments.
 */
struct server_cmdline_arguments {
    int port;
};

/*
 * struct for storing client command line arguments.
 */
struct client_cmdline_arguments {
    // server address
    char *server_ip;

    // server port
    int server_port;

    // time format
    char *time_format;
};

const struct server_cmdline_arguments *parse_server_arguments(int argc, char *argv[]);
const struct client_cmdline_arguments *parse_client_arguments(int argc, char *argv[]);
