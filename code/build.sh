#!/bin/sh

set -e
cd $(dirname $0)

docker run --rm \
        -v $(pwd):/my_plugin etriphany/fluent-bit-plugin-dev \
        sh -c "cmake -DFLB_SOURCE=/usr/src/fluentbit/fluent-bit-1.8.4/ -DPLUGIN_NAME=in_docker_stats ../ && make"