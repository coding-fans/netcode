/*
 * Author: fasion
 * Created time: 2021-05-13 18:39:50
 * Last Modified by: fasion
 * Last Modified time: 2022-02-28 11:51:55
 */

#include <argp.h>

#include "argparse.h"


/**
 * server opt_handler function for GNU argp.
 **/
static error_t server_opt_handler(int key, char *arg, struct argp_state *state) {
    struct server_cmdline_arguments *arguments = state->input;

    switch(key) {
        case 'i':
            arguments->bind_ip = arg;
            break;

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
    static char const doc[] = "server: tcp upper server";
    static char const args_doc[] = "";

    // command line options
    static struct argp_option const options[] = {
        // Option -i --bind-ip: server address
        {"bind-ip", 'i', "BIND_IP", 0, "bind ip"},

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
        .bind_ip = "0.0.0.0",
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
    static char const doc[] = "client: tcp upper client";
    static char const args_doc[] = "";

    // command line options
    static struct argp_option const options[] = {
        // Option -a --address: server address
        {"server-ip", 'i', "SERVER_IP", 0, "server ip"},

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
    };

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    return &arguments;
}
