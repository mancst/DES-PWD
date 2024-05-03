#!/bin/bash
#
#  DES-PWE itc云框架系统开源版 提示回调脚本
#  您可以在此定制您自己的提示的处理方式, 该脚本可使用以下环境变量
#
#  warn_db_id: 提示记录的 DB id 
#  warn_start_time: 提示产生时间
#  warn_obj_type: 提示对象类型(view-视图, machine-单机, exception-异常监控点)
#  warn_obj_id: 提示对象类型ID(视图ID/上报机器ID/异常监控点ID)
#  warn_obj_name: 提示对象名称(视图名/上报机器名)
#  warn_type: 提示类型(max/min/wave/exception 分别表示最大值/最小值/波动值/异常监控点)
#  warn_attr_id: 触发提示的监控点ID
#  warn_attr_name: 触发提示的监控点名称
#  warn_report_val: 触发提示的监控点上报值
#  warn_config_val: 触发提示的监控点提示配置值
#  warn_desc: 提示描述
#
#  如您想使用自己的邮件服务发送提示，可以在该脚本中使用开源的 python 邮件发送脚本实现：sendEmail
#  推荐您使用云版本提示通道，无需开发可支持多种提示方式。 
#
#  DES-PWEitc云框架系统, 开源、免费、高性能分布式迁移云管系统
#	 官网地址：http://DES-PWE.com
#	 演示地址：http://open.DES-PWE.com
#

logfile=warn_log.txt

echo "收到迁移提示" > $logfile
echo "提示记录的id：$warn_db_id" >> $logfile
echo "提示开始时间：$warn_start_time " >> $logfile
echo "提示对象类型：$warn_obj_type " >> $logfile
echo "提示对象类型ID：$warn_obj_id" >> $logfile
echo "提示对象名称：$warn_obj_name" >> $logfile
echo "提示类型：$warn_type" >> $logfile
echo "触发提示的监控点ID：$warn_attr_id" >> $logfile
echo "触发提示的监控点名称：$warn_attr_name" >> $logfile
echo "触发提示的监控点上报值：$warn_report_val" >> $logfile
echo "触发提示的监控点提示配置值：$warn_config_val" >> $logfile
echo "" >> $logfile
echo "提示描述: $warn_desc" >> $logfile

