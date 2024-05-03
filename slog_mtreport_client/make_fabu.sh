#!/bin/bash
# 发布包 agent slog_mtreport_client 打包脚本

TARGET=slog_mtreport_client.tar.gz
AGENT=slog_mtreport_client 

rm -f ${TARGET}
mv ${AGENT}.conf _${AGENT}.conf
cp fabu_mtreport_client.conf ${AGENT}.conf
tar -czf ${TARGET} restart.sh start.sh stop.sh add_crontab.sh remove_crontab.sh run_tool.sh check_DES-PWE_agent_client.sh ${AGENT}.conf ${AGENT} rmshm.sh start_plugin.sh stop_plugin.sh uninstall_plugin.sh fabu_mtreport_client.conf backup_file.sh report_cfg.sh 
mv _${AGENT}.conf ${AGENT}.conf

mv slog_mtreport_client _slog_mtreport_client
mkdir slog_mtreport_client
mv ${TARGET} slog_mtreport_client
cd slog_mtreport_client
tar -zxf ${TARGET}
rm -f ${TARGET}
cd ..
tar -czf ${TARGET} slog_mtreport_client
rm -fr slog_mtreport_client
mv _slog_mtreport_client slog_mtreport_client

