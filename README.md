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
./test.sh
 ```

# Design

## Required volumes

This plugin requires some volumes to collect metrics from containers running in the `host node`.

It maps the folder `/var/lib/docker/containers` so it can simply know all containers' ids running in the underline host without any request/response to Docker Engine API.

It also maps the **host unix socket** `/var/run/docker.sock`, so it can send requests to the Docker Engine API to fetch data for containers retrieved previously from mounted `/var/lib/docker/containers`.

> This plugin WILL NOT WORK without both volumes properly mounted on `Fluentbit` container

## Collecting data
The plugin will access the Docker Engine endpoint `/containers/{id}/stats`.

It will use the following parameters:
 * stream=false (avoids socket streaming, and send just a single value)
 * one-shot=false (can't use single shot in order to recover `precpu_stats`)

Each response will include important information like:

 - Container CPU stats
 - Container memory stats
 - Container network stats
 - Container disk stats

## Configurations

This input plugin can be configured using the following parameters:

| Key  | Description | Default |
| ---- | ----------- | ------ |
| Collect_Interval | Interval in seconds to collect data  | 10 |
| Unix_Path | Define target Unix socket path. | /var/run/docker.sock
| Containers_Path | Define the folder that contains containers' ids. | /var/lib/docker/containers |
| Buffer_Size | The size of the buffer used to read data (in bytes or [unit sized](https://docs.fluentbit.io/manual/administration/configuring-fluent-bit/unit-sizes)) | 8192 |
| Parser | Specify the name of a parser to interpret the entry as a structured message. | None |
| Key | When a message is unstructured (no parser applied), it's appended as a string under the key name message. | message |

