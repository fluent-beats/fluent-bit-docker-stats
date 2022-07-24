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

#include <fluent-bit/flb_input_plugin.h>
#include <fluent-bit/flb_network.h>
#include <fluent-bit/flb_pack.h>
#include <msgpack.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <dirent.h>

#include "docker_stats.h"
#include "docker_stats_config.h"

static container_info *init_container_info(char *id)
{
    container_info *container;

    container = flb_malloc(sizeof(container_info));
    if (!container) {
        flb_errno();
        return NULL;
    }

    container->id = flb_malloc(sizeof(char)*(DOCKER_SHORT_ID_LEN + 1));
    if (!container->id) {
        flb_errno();
        flb_free(container);
        return NULL;
    }
    strncpy(container->id, id, DOCKER_SHORT_ID_LEN);
    container->id[DOCKER_SHORT_ID_LEN] = '\0';

    return container;
}

/**
 * Traverse /var/lib/docker/containers folder to pick container
 *
 * @param ctx  Pointer to flb_in_dstats_config
 *
 * @return mk_list with all retrieved containers
 */
static struct mk_list *get_containers_info(struct flb_in_dstats_config *ctx)
{
    DIR *dp;
    struct dirent *ep;
    struct mk_list *list;

    list = flb_malloc(sizeof(struct mk_list));
    if (!list) {
        flb_errno();
        return NULL;
    }
    mk_list_init(list);

    dp = opendir(ctx->containers_path);
    if (dp != NULL) {

        while ((ep = readdir(dp)) != NULL) {
            if (ep->d_type == OS_DIR_TYPE) {
                if (strlen(ep->d_name) == DOCKER_LONG_ID_LEN) {
                    container_info *container = init_container_info(ep->d_name);
                    mk_list_add(&container->_head, list);
                }
            }
        }
        closedir(dp);
    }

    return list;
}

/**
 * Sends request to docker's unix socket
 * HTTP GET /containers/{ID}/stats?stream=false&one-shot=true
 *
 * @param ctx  Pointer to flb_in_dstats_config
 * @param container_id Unique docker container id
 *
 * @return int 0 on success, -1 on failure
 */
static void dstats_unix_socket_write(struct flb_in_dstats_config *ctx,
                                     char* container_id)
{
    char request[512];

    snprintf(request, sizeof(request), "GET /containers/%s/stats?stream=false HTTP/1.0\r\n\r\n", container_id);
    flb_plg_trace(ctx->ins, "send request %s", request);

    /* Write request */
    write(ctx->fd, request, strlen(request));

    return;
}

/**
 * Read response from docker's unix socket.
 *
 * @param ins           Pointer to flb_input_instance
 * @param config        Pointer to flb_config
 * @param in_context    void Pointer used to cast to flb_in_dstats_config
 *
 * @return int Always returns success
 */
static int dstats_unix_socket_read(struct flb_input_instance *ins,
                                   struct flb_config *config, void *in_context)
{
    int ret = 0;
    int error;
    char *body;
    size_t str_len = 0;
    struct flb_in_dstats_config *ctx = in_context;
    msgpack_packer mp_pck;
    msgpack_sbuffer mp_sbuf;

    /* variables for parser */
    int parser_ret = -1;
    void *out_buf = NULL;
    size_t out_size = 0;
    struct flb_time out_time;

    /* Read response */
    ret = read(ctx->fd, ctx->buf, ctx->buf_size - 1);

    if (ret > 0) {
        str_len = ret;
        ctx->buf[str_len] = '\0';

        /* Skip HTTP headers */
        body = strstr(ctx->buf, HTTP_BODY_DELIMITER);
        body += strlen(HTTP_BODY_DELIMITER);
        str_len = strlen(body);

        if (!ctx->parser) {
            /* Initialize local msgpack buffer */
            msgpack_sbuffer_init(&mp_sbuf);
            msgpack_packer_init(&mp_pck, &mp_sbuf, msgpack_sbuffer_write);

            msgpack_pack_array(&mp_pck, 2);
            flb_pack_time_now(&mp_pck);
            msgpack_pack_map(&mp_pck, 1);

            msgpack_pack_str(&mp_pck, flb_sds_len(ctx->key));
            msgpack_pack_str_body(&mp_pck, ctx->key, flb_sds_len(ctx->key));
            msgpack_pack_str(&mp_pck, str_len);
            msgpack_pack_str_body(&mp_pck, body, str_len);
            flb_input_chunk_append_raw(ins, NULL, 0, mp_sbuf.data, mp_sbuf.size);
            msgpack_sbuffer_destroy(&mp_sbuf);
        }
        else {
            flb_time_get(&out_time);
            parser_ret = flb_parser_do(ctx->parser, body, str_len - 1,
                                       &out_buf, &out_size, &out_time);
            if (parser_ret >= 0) {
                if (flb_time_to_double(&out_time) == 0.0) {
                    flb_time_get(&out_time);
                }

                /* Initialize local msgpack buffer */
                msgpack_sbuffer_init(&mp_sbuf);
                msgpack_packer_init(&mp_pck, &mp_sbuf, msgpack_sbuffer_write);

                msgpack_pack_array(&mp_pck, 2);
                flb_time_append_to_msgpack(&out_time, &mp_pck, 0);
                msgpack_sbuffer_write(&mp_sbuf, out_buf, out_size);

                flb_input_chunk_append_raw(ins, NULL, 0,mp_sbuf.data, mp_sbuf.size);
                msgpack_sbuffer_destroy(&mp_sbuf);
                flb_free(out_buf);
            }
            else {
                flb_plg_trace(ctx->ins, "tried to parse: %s", ctx->buf);
                flb_plg_trace(ctx->ins, "buf_size %zu", ctx->buf_size);
                flb_plg_error(ctx->ins, "parser returned an error: %d", parser_ret);
            }
        }
    }
    else if (ret < 0) {
        error = errno;
        flb_plg_error(ctx->ins, "socket error: %d, %s", error, strerror(error));
    }

    return 0;
}

/**
 * Creates the connection to docker's unix socket
 *
 * @param ctx  Pointer to flb_in_dstats_config
 *
 * @return int 0 on success, -1 on failure
 */
static int dstats_unix_socket_connect(struct flb_in_dstats_config *ctx)
{
    unsigned long len;
    size_t address_length;
    struct sockaddr_un address;

    /* Disconnect */
    if(ctx->fd > 0) {
        flb_plg_trace(ctx->ins, "closed socket fd=%d", ctx->fd);
        close(ctx->fd);
        ctx->fd = -1;
    }

    /* Create */
    if (ctx->fd < 0) {
        ctx->fd = flb_net_socket_create(AF_UNIX, FLB_FALSE);
        if (ctx->fd == -1) {
            return -1;
        }
    }

    /* Prepare the unix socket path */
    len = strlen(ctx->unix_path);
    address.sun_family = AF_UNIX;
    sprintf(address.sun_path, "%s", ctx->unix_path);
    address_length = sizeof(address.sun_family) + len + 1;

    /* Connect */
    if (connect(ctx->fd, (struct sockaddr *)&address, address_length) == -1) {
        flb_errno();
        close(ctx->fd);
        return -1;
    }

    return 0;
}

/**
 * Callback function to interact with docker engine API.
 *
 * @param ins           Pointer to flb_input_instance
 * @param config        Pointer to flb_config
 * @param in_context    void Pointer used to cast to
 *                      flb_in_dstats_config
 *
 * @return int Always returns success
 */
static int cb_dstats_collect(struct flb_input_instance *ins,
                             struct flb_config *config, void *in_context)
{
    struct flb_in_dstats_config *ctx = in_context;
    struct mk_list *containers;
    struct mk_list *head;
    struct mk_list *tmp;
    container_info *container;

    containers = get_containers_info(ctx);
    mk_list_foreach_safe(head, tmp, containers) {
        container = mk_list_entry(head, container_info, _head);

        dstats_unix_socket_connect(ctx);
        dstats_unix_socket_write(ctx, container->id);
        dstats_unix_socket_read(ins, config, in_context);
    }

    return 0;
}

/**
 * Callback function to initialize docker stats plugin
 *
 * @param ins     Pointer to flb_input_instance
 * @param config  Pointer to flb_config
 * @param data    Unused
 *
 * @return int 0 on success, -1 on failure
 */
static int in_dstats_init(struct flb_input_instance *ins,
                          struct flb_config *config, void *data)
{
    struct flb_in_dstats_config *ctx = NULL;
    (void) data;

    /* Allocate space for the configuration */
    ctx = dstats_config_init(ins, config);
    if (!ctx) {
        return -1;
    }
    ctx->ins = ins;
    ctx->fd = -1;

    /* Set the context */
    flb_input_set_context(ins, ctx);

    /* Set our collector based on time */
    ctx->coll_id = flb_input_set_collector_time(ins, cb_dstats_collect,
                                                ctx->collect_interval,
                                                0, config);
    if(ctx->coll_id < 0){
        flb_plg_error(ctx->ins, "could not set collector for IN_DOCKER_STATS plugin");
        dstats_config_destroy(ctx);
        return -1;
    }

    return 0;
}

/**
 * Callback exit function to cleanup plugin
 *
 * @param data    Pointer cast to flb_in_dstats_config
 * @param config  Unused
 *
 * @return int    Always returns 0
 */
static int in_dstats_exit(void *data, struct flb_config *config)
{
    (void) config;
    struct flb_in_dstats_config *ctx = data;

    if (!ctx) {
        return 0;
    }

    dstats_config_destroy(ctx);

    return 0;
}

/* Configuration properties map */
static struct flb_config_map config_map[] = {
    {
     FLB_CONFIG_MAP_TIME, "collect_interval", "10",
     0, FLB_TRUE, offsetof(struct flb_in_dstats_config, collect_interval),
     "Stats collection interval."
    },
    {
     FLB_CONFIG_MAP_STR, "unix_path", DEFAULT_UNIX_SOCKET_PATH,
     0, FLB_TRUE, offsetof(struct flb_in_dstats_config, unix_path),
     "Define Docker unix socket path to read events"
    },
    {
     FLB_CONFIG_MAP_STR, "containers_path", DEFAULT_DOCKER_LIB_ROOT,
     0, FLB_TRUE, offsetof(struct flb_in_dstats_config, containers_path),
     "Define Docker unix socket path to read events"
    },
    {
     FLB_CONFIG_MAP_SIZE, "buffer_size", "8k",
     0, FLB_TRUE, offsetof(struct flb_in_dstats_config, buf_size),
     "Set buffer size to read events"
    },
    {
     FLB_CONFIG_MAP_STR, "parser", NULL,
      0, FLB_FALSE, 0,
     "Optional parser for records, if not set, records are packages under 'key'"
    },
    {
     FLB_CONFIG_MAP_STR, "key", DEFAULT_FIELD_NAME,
     0, FLB_TRUE, offsetof(struct flb_in_dstats_config, key),
     "Set the key name to store unparsed Docker events"
    },
    /* EOF */
    {0}
};

/* Plugin reference */
struct flb_input_plugin in_docker_stats_plugin = {
    .name         = "docker_stats",
    .description  = "Docker stats",
    .cb_init      = in_dstats_init,
    .cb_pre_run   = NULL,
    .cb_collect   = cb_dstats_collect,
    .cb_flush_buf = NULL,
    .cb_exit      = in_dstats_exit,
    .config_map   = config_map,
    .flags        = FLB_INPUT_NET
};
