/*
 * Author: fasion
 * Created time: 2021-05-13 18:39:13
 * Last Modified by: fasion
 * Last Modified time: 2022-02-28 09:39:29
 */

/*
 * struct for storing server command line arguments.
 */
struct server_cmdline_arguments {
    // bind address
    char *bind_ip;

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
};

const struct server_cmdline_arguments *parse_server_arguments(int argc, char *argv[]);
const struct client_cmdline_arguments *parse_client_arguments(int argc, char *argv[]);
