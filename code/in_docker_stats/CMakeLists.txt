set(src
  docker_stats.c
  docker_stats_config.c)

FLB_PLUGIN(in_docker_stats "${src}" "")
