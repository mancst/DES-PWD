#!/bin/bash
PATH=/bin:/sbin:/usr/bin/:/usr/sbin:/usr/local/bin:/usr/local/sbin:/usr/local/mysql/bin
# ---------------------------------------------------------------------------------
# DES-PWE migration (c) 2023 by itc
# License Agreement： apache license 2.0
#

#
# 云版本为开源版提供永久免费提示通道支持，提示通道支持短信、邮件、
# 微信等多种方式，欢迎使用
# ---------------------------------------------------------------------------------
#
# 当前脚本说明:
# 卸载云框架系统, 相关配置参数通过安装脚本生成, 请勿修改
#
#


APACHE_DOCUMENT_ROOT=/srv/www/htdocs
APACHE_CGI_PATH=/srv/www/cgi-bin/
DES-PWE_HTML_PATH=DES-PWE
DES-PWE_CGI_LOG_PATH=/var/log/mtreport
MYSQL_USER=root
MYSQL_PASS=123
SLOG_SERVER_FILE_PATH=/home/mtreport/slog/
SYSTEM_LIB_PATH=/usr/lib64
XRKLIB_INSTALL_HIS_FILE=_DES-PWE_lib_install

function yn_continue()
{
	read -p "$1" op
	while [ "$op" != "Y" -a "$op" != "y" -a "$op" != "N" -a "$op" != "n" ]; do
		read -p "请输入 (y/n): " op
	done
	if [ "$op" != "y" -a "$op" != "Y" ];then
		echo "no" 
	else
		echo "yes"
	fi
}

isunins=$(yn_continue "确认卸载云框架系统吗(y/n) ?")
if [ "$isunins" != "yes" ]; then 
	exit 0 
fi

ls -l /tmp/pid*slog*pid >/dev/null 2>&1
if [ $? -eq 0 -a -f tools_sh/stop_all.sh -a -f tools_sh/rm_zero.sh ]; then
	echo "开始停止云框架系统服务, 请耐心等待..."
	cd tools_sh; ./stop_all.sh; 
	echo "开始清理共享内存"
	./rm_zero.sh
	cd ..
	rm -f /tmp/pid*slog*pid > /dev/null 2>&1
fi

if [ -f /tmp/_slog_config_read_ok ]; then
	rm -f /tmp/_slog_config_read_ok
fi

if [ -f $APACHE_DOCUMENT_ROOT/index.html ]; then
	cat $APACHE_DOCUMENT_ROOT/index.html |grep "$DES-PWE_HTML_PATH" > /dev/null 2>&1
	if [ $? -eq 0 ]; then
		echo "删除文件: $APACHE_DOCUMENT_ROOT/index.html"
		rm -f $APACHE_DOCUMENT_ROOT/index.html
	fi
fi

if [ -f $APACHE_DOCUMENT_ROOT/$DES-PWE_HTML_PATH/dmt_login.html ]; then
	echo "删除 html/js 文件目录: $APACHE_DOCUMENT_ROOT/$DES-PWE_HTML_PATH"
	rm -fr $APACHE_DOCUMENT_ROOT/$DES-PWE_HTML_PATH
fi

if [ -f "$APACHE_CGI_PATH/mt_slog" ]; then
	echo "删除 cgi 文件"
	rm -f $APACHE_CGI_PATH/mt_slog*
	rm -f $APACHE_CGI_PATH/slog_flogin*
fi

[ -d "$DES-PWE_CGI_LOG_PATH" ] && (echo "删除 cgi 日志目录: $DES-PWE_CGI_LOG_PATH"; rm -fr "$DES-PWE_CGI_LOG_PATH")
[ -d slog_core ] && (echo "删除目录: slog_core"; rm -fr slog_core)
[ -d slog_check_proc ] && (echo "删除目录: slog_check_proc"; rm -fr slog_check_proc)
[ -d DES-PWE_lib ] && (echo "删除目录: DES-PWE_lib"; rm -fr DES-PWE_lib)
[ -d cgi_fcgi ] && (echo "删除目录: cgi_fcgi"; rm -fr cgi_fcgi)
[ -d db ] && (echo "删除目录: db"; rm -fr db)
[ -d html ] && (echo "删除目录: html"; rm -fr html)
[ -d slog_check_warn ] && (echo "删除目录: slog_check_warn"; rm -fr slog_check_warn)
[ -d slog_client ] && (echo "删除目录: slog_client"; rm -fr slog_client)
[ -d slog_config ] && (echo "删除目录: slog_config"; rm -fr slog_config)
[ -d slog_deal_warn ] && (echo "删除目录: slog_deal_warn"; rm -fr slog_deal_warn)
[ -d slog_memcached ] && (echo "删除目录: slog_memcached"; rm -fr slog_memcached)
[ -d slog_monitor_server ] && (echo "删除目录: slog_monitor_server"; rm -fr slog_monitor_server)
[ -d slog_mtreport_client ] && (echo "删除目录: slog_mtreport_client"; rm -fr slog_mtreport_client)
[ -d slog_mtreport_server ] && (echo "删除目录: slog_mtreport_server"; rm -fr slog_mtreport_server)
[ -d slog_server ] && (echo "删除目录: slog_server"; rm -fr slog_server)
[ -d slog_tool ] && (echo "删除目录: slog_tool"; rm -fr slog_tool)
[ -d slog_write ] && (echo "删除目录: slog_write"; rm -fr slog_write)
[ -d tools_sh ] && (echo "删除目录: tools_sh"; rm -fr tools_sh)
[ -f _run_test_tmp ] && (echo "删除临时文件: _run_test_tmp"; rm -fr _run_test_tmp)
[ -f slog_run_test ] && (echo "删除测试文件: slog_run_test"; rm -fr slog_run_test)

function yn_continue()
{
	read -p "$1" op
	while [ $op != "Y" -a $op != "y" -a $op != "N" -a $op != "n" ]; do
		read -p "请输入 (y/n): " op
	done
	if [ "$op" != "y" -a "$op" != "Y" ];then
		echo "no" 
	else
		echo "yes"
	fi
}

echo "删除库文件"
if [ ! -f /etc/ld.so.conf ]; then
	echo "动态链接库配置文件: /etc/ld.so.conf 不存在!"
else
	cat /etc/ld.so.conf |grep DES-PWE_lib > /dev/null 2>&1
	if [ $? -eq 0 ]; then
		sed -i '/DES-PWE_lib/d' /etc/ld.so.conf
		ldconfig
	fi
fi

function remove_old_lib()
{
	slib=$1
	slk=$2
	LIBINFO=`cat ${SYSTEM_LIB_PATH}/$XRKLIB_INSTALL_HIS_FILE | grep $slib`
	if [ "$LIBINFO" != '' ]; then
		rm ${SYSTEM_LIB_PATH}/$slib -f
	fi

	LIBINFO=`cat ${SYSTEM_LIB_PATH}/$XRKLIB_INSTALL_HIS_FILE | grep $slk`
	if [ "$LIBINFO" != '' ]; then
		rm ${SYSTEM_LIB_PATH}/$slk -f
		echo "删除库文件：${SYSTEM_LIB_PATH}/$slib"
	fi
}
if [ -f ${SYSTEM_LIB_PATH}/$XRKLIB_INSTALL_HIS_FILE ]; then
	remove_old_lib libSockets-1.1.0.so libSockets.so.1
	remove_old_lib libcgicomm-1.1.0.so libcgicomm.so.1
	remove_old_lib libfcgi.so.0.0.0 libfcgi.so.0
	remove_old_lib libmtreport_api-1.1.0.so libmtreport_api.so.1
	remove_old_lib libmtreport_api_open-1.1.0.so libmtreport_api_open.so.1
	remove_old_lib libmyproto-1.1.0.so libmyproto.so.1
	remove_old_lib libmysqlwrapped-1.1.0.so libmysqlwrapped.so.1
	remove_old_lib libneo_cgi-1.1.0.so libneo_cgi.so.1
	remove_old_lib libneo_cs-1.1.0.so libneo_cs.so.1
	remove_old_lib libneo_utl-1.1.0.so libneo_utl.so.1
	remove_old_lib libprotobuf.so.6.0.0 libprotobuf.so.6
	rm -f ${SYSTEM_LIB_PATH}/$XRKLIB_INSTALL_HIS_FILE > /dev/null 2>&1 
fi


echo ""
if [ -z "$MYSQL_USER" -o -z "$MYSQL_PASS" ]; then
	MYSQL_CONTEXT="mysql -B "
else 
	MYSQL_CONTEXT="mysql -B -u$MYSQL_USER -p$MYSQL_PASS"
fi

echo "show databases" | ${MYSQL_CONTEXT} |grep mtreport_db > /dev/null 2>&1
if [ $? -eq 0 ]; then
	isyes=$(yn_continue "是否清理 mysql 数据库 mtreport_db/attr_db(y/n)?")
	if [ "$isyes" == "yes" ]; then
		echo "删除 mysql 数据库 mtreport_db/attr_db"
		echo "drop database mtreport_db" | ${MYSQL_CONTEXT}
		echo "drop database attr_db" | ${MYSQL_CONTEXT}
		echo "删除默认 mysql 账号 mtreport"
		echo "drop user mtreport@localhost" | ${MYSQL_CONTEXT}
	fi
else
	echo "未检测到 mysql 数据库: mtreport_db/attr_db, 跳过数据库清理"	
fi

crontab -l > _del_DES-PWE_proc_check
grep "check_proc_monitor.sh" _del_DES-PWE_proc_check > /dev/null 2>&1
if [ $? -eq 0 ]; then
	echo "移除 crontab 监控脚本"
	sed -i '/check_proc_monitor\.sh/d' _del_DES-PWE_proc_check
	crontab _del_DES-PWE_proc_check
fi
rm -f _del_DES-PWE_proc_check

if [ -d "$SLOG_SERVER_FILE_PATH" ]; then
	isyes=$(yn_continue "是否删除日志中心目录以及日志文件 (y/n)?")
	if [ "$isyes" == "yes" ]; then
		rm -fr $SLOG_SERVER_FILE_PATH
	fi
else
	echo "未检测到日志中心日志文件目录: $SLOG_SERVER_FILE_PATH, 跳过日志清理"
fi

isyes=$(yn_continue "是否删除安装/卸载脚本 (y/n)?")
if [ "$isyes" == "yes" ]; then
	rm -f local_install.sh > /dev/null 2>&1
	rm -f uninstall_DES-PWE.sh > /dev/null 2>&1
fi

isyes=$(yn_continue "是否删除安装包 (y/n)?")
if [ "$isyes" == "yes" ]; then
	[ -f slog_all.tar.gz ] && (echo "删除测试文件: slog_all.tar.gz"; rm -fr slog_all.tar.gz)
	[ -f DES-PWE_lib.tar.gz ] && (echo "删除测试文件: DES-PWE_lib.tar.gz"; rm -fr DES-PWE_lib.tar.gz)
fi

echo "已为您清理干净云框架系统安装记录, 感谢您的关注."
echo ""

