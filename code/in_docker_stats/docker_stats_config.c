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
#include <fluent-bit/flb_utils.h>

#include "docker_stats.h"
#include "docker_stats_config.h"

/**
 * Function to initialize docker_stats plugin.
 *
 * @param ins     Pointer to flb_input_instance
 * @param config  Pointer to flb_config
 *
 * @return struct flb_in_dstats_config* Pointer to the plugin's
 *         structure on success, NULL on failure.
 */
struct flb_in_dstats_config *dstats_config_init(struct flb_input_instance *ins,
                                                struct flb_config *config)
{
    int ret;
    const char *tmp;
    struct flb_in_dstats_config *ctx;

    ctx = flb_calloc(1, sizeof(struct flb_in_dstats_config));
    if (!ctx) {
        flb_errno();
        return NULL;
    }
    ctx->ins = ins;

    /* Load the config map */
    ret = flb_input_config_map_set(ins, (void *) ctx);
    if (ret == -1) {
        flb_free(ctx);
        return NULL;
    }

    /* Allocate buffer for stats */
    ctx->buf = flb_malloc(ctx->buf_size);
    if (!ctx->buf) {
        flb_errno();
        flb_free(ctx);
        return NULL;
    }

    /* Optional parser */
    tmp = flb_input_get_property("parser", ins);
    if (tmp) {
        ctx->parser = flb_parser_get(tmp, config);
        if (ctx->parser == NULL) {
            flb_plg_error(ctx->ins, "requested parser '%s' not found", tmp);
            flb_free(ctx->buf);
            flb_free(ctx);
            return NULL;
        }
    }

    return ctx;
}

/**
 * Function to destroy docker_stats plugin.
 *
 * @param ctx  Pointer to flb_in_dstats_config
 *
 * @return int 0
 */
int dstats_config_destroy(struct flb_in_dstats_config *ctx)
{
    if (ctx->buf) {
        flb_free(ctx->buf);
    }

    flb_free(ctx);
    return 0;
}
