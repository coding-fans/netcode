/*
 * Author: fasion
 * Created time: 2020-10-27 20:22:53
 * Last Modified by: fasion
 * Last Modified time: 2021-02-24 16:51:57
 */

#include <argp.h>

#include "argparse.h"


/**
 * server opt_handler function for GNU argp.
 **/
static error_t server_opt_handler(int key, char *arg, struct argp_state *state) {
    struct server_cmdline_arguments *arguments = state->input;

    switch(key) {
        case 'p':
            if (sscanf(arg, "%d", &arguments->port) != 1) {
                return ARGP_ERR_UNKNOWN;
            }
            break;

        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}


/**
 * Parse server command line arguments given by argc, argv.
 *
 *  Arguments
 *      argc: the same with main function.
 *
 *      argv: the same with main function.
 *
 *  Returns
 *      Pointer to struct server_cmdline_arguments if success, NULL if error.
 **/
const struct server_cmdline_arguments *parse_server_arguments(int argc, char *argv[]) {
    // docs for program and options
    static char const doc[] = "server: udp time server";
    static char const args_doc[] = "";

    // command line options
    static struct argp_option const options[] = {
        // Option -p --port: listen port
        {"port", 'p', "PORT", 0, "listen port"},

        { 0 }
    };

    static const struct argp argp = {
        options,
        server_opt_handler,
        args_doc,
        doc,
        0,
        0,
        0,
    };

    // for storing results
    static struct server_cmdline_arguments arguments = {
        .port = 9999,
    };

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    return &arguments;
}


/**
 * client opt_handler function for GNU argp.
 **/
static error_t client_opt_handler(int key, char *arg, struct argp_state *state) {
    struct client_cmdline_arguments *arguments = state->input;

    switch(key) {
        case 'i':
            arguments->server_ip = arg;
            break;

        case 'f':
            arguments->time_format = arg;
            break;

        case 'p':
            if (sscanf(arg, "%d", &arguments->server_port) != 1) {
                return ARGP_ERR_UNKNOWN;
            }
            break;

        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}


/**
 * Parse client command line arguments given by argc, argv.
 *
 *  Arguments
 *      argc: the same with main function.
 *
 *      argv: the same with main function.
 *
 *  Returns
 *      Pointer to struct client_cmdline_arguments if success, NULL if error.
 **/
const struct client_cmdline_arguments *parse_client_arguments(int argc, char *argv[]) {
    // docs for program and options
    static char const doc[] = "client: udp time client";
    static char const args_doc[] = "";

    // command line options
    static struct argp_option const options[] = {
        // Option -a --address: server address
        {"server-ip", 'i', "SERVER_IP", 0, "server ip"},

        // Option -f --time-format: time format
        {"time-format", 'f', "TIME_FORMAT", 0, "time format"},

        // Option -p --port: server port
        {"server-port", 'p', "SERVER_PORT", 0, "listen port"},

        { 0 }
    };

    static const struct argp argp = {
        options,
        client_opt_handler,
        args_doc,
        doc,
        0,
        0,
        0,
    };

    // for storing results
    static struct client_cmdline_arguments arguments = {
        .server_ip = "127.0.0.1",
        .server_port = 9999,
        .time_format = "%Y-%m-%d %H:%M:%S",
    };

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    return &arguments;
}
