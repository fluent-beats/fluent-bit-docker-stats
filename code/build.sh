#!/bin/sh

set -e
cd $(dirname $0)

docker run --rm \
        -v $(pwd):/my_plugin ghcr.io/fluent-beats/docker-flb-plugin-sdk:1.1.0 \
        sh -c "cmake -DFLB_SOURCE=/usr/src/fluentbit/fluent-bit-1.8.11/ -DPLUGIN_NAME=in_docker_stats ../ && make"