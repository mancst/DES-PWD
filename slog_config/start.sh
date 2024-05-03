#!/bin/bash
if [ -f ipinfo.tar.gz -a ! -f DES-PWE_ip_info ]; then
    tar -zxf ipinfo.tar.gz
fi
. ../tools_sh/start_comm.sh
start_proc slog_config

