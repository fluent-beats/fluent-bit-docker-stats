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

#ifndef FLB_IN_DSTATS_CONFIG_H
#define FLB_IN_DSTATS_CONFIG_H

#include "docker_stats.h"

struct flb_in_dstats_config *dstats_config_init(struct flb_input_instance *ins,
                                                struct flb_config *config);
int dstats_config_destroy(struct flb_in_dstats_config *config);

#endif
