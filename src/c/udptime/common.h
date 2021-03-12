/*
 * Author: fasion
 * Created time: 2021-02-23 19:00:35
 * Last Modified by: fasion
 * Last Modified time: 2021-02-24 19:05:53
 */

#include <stdint.h>

#define MAX_FORMAT_SIZE 1024
#define MAX_DATA_SIZE 1024

/*
 * struct for time request.
 */
struct __attribute__((__packed__)) time_request {
    uint32_t bytes;
    char format[MAX_FORMAT_SIZE];
};

/*
 * struct for time reply.
 */
struct __attribute__((__packed__)) time_reply {
    uint32_t bytes;
    char time[MAX_DATA_SIZE];
};
