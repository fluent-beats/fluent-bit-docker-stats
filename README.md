# Description

[Fluent Bit](https://fluentbit.io) input plugin able to access [Docker Container Stats](https://docs.docker.com/engine/api/v1.41/#operation/ContainerStats)

# Requirements

- Docker
- Docker image `fluent-beats/fluent-bit-plugin-dev`

# Build
```bash
./build.sh
```

# Test
```bash
docker run --rm \
      -v /var/run/docker.sock:/var/run/docker.sock:ro \
      -v /var/lib/docker/containers:/var/lib/docker/containers \
      -v $(pwd)/code/build:/my_plugin \
      fluent/fluent-bit:1.8.4 /fluent-bit/bin/fluent-bit -e /my_plugin/flb-in_docker_stats.so -i docker_stats -o stdout
 ```

 # Design

 ## Required volumes

 `Fluentbit` provides very simple libraries to deal with JSON processing, so to make this plugin bare simple it uses a simple strategy to collect stats from all container running in the `host node`.

 First it maps the folder `/var/lib/docker/containers` so it can simply know all containers' ids running in the underline host without any request/response to Docker Engine API.

 Second it maps the **host unix socket** `/var/run/docker.sock`, so it can send requests to the Docker Engine API to fetch stats from all containers retrieved previously from mounted `/var/lib/docker/containers`.

> This plugin WILL NOT WORK without both volumes properly mounted on `Fluentbit` container

## Collecting stats
 The plugin will access the Docker Engine endpoint `/containers/{id}/stats`.

 It will use 2 parameters to improve performance and save rescources:
 * stream=false (avoids socket streaming, and send just a single value)
 * one-shot=true (faster retrival on single shot)

 For each response the plugin will simply send the JSON values to the output without any transformation or parsing, saving even more memory and resources.