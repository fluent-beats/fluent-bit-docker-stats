/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*  Etriphany
 *  ==========
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef FLB_IN_DE_H
#define FLB_IN_DE_H

#include <msgpack.h>
#include <fluent-bit/flb_input.h>
#include <fluent-bit/flb_parser.h>
#include <monkey/mk_core/mk_list.h>

#define OS_DIR_TYPE                 4
#define DOCKER_LONG_ID_LEN          64
#define DOCKER_SHORT_ID_LEN         12
#define DEFAULT_BUF_SIZE            "8192"
#define MIN_BUF_SIZE                "2048"
#define DEFAULT_FIELD_NAME          "message"
#define DEFAULT_UNIX_SOCKET_PATH    "/var/run/docker.sock"
#define DEFAULT_DOCKER_LIB_ROOT     "/var/lib/docker/containers"
#define HTTP_BODY_DELIMITER         "\r\n\r\n"

/* Method added latter on Fluenbit under macro FLB_INPUT_RETURN */
static inline void flb_input_return_do(int ret) {
    struct flb_coro *coro = flb_coro_get();

    flb_input_return(coro);
    flb_coro_yield(coro, FLB_TRUE);
}

#define FLB_INPUT_RETURN(x) \
    flb_input_return_do(x); \
    return x;

typedef struct container_info {
    char *id;
    struct mk_list _head;
} container_info;

struct flb_in_dstats_config
{
    int fd;                         /* File descriptor */
    int coll_id;                    /* collector id */
    flb_sds_t unix_path;            /* Unix path for socket */
    flb_sds_t containers_path;      /* Unix path for containers lib directory */
    uint64_t collect_interval;
    char *buf;
    size_t buf_size;
    flb_sds_t key;

    struct flb_parser *parser;
    struct flb_input_instance *ins; /* Input plugin instace */

};

#endif
