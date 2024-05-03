#!/bin/bash

USE_DLL_COMM_LIB=`cat ../make_env |grep ^USE_DLL_COMM_LIB|awk '{print $3}'`
if [ "${USE_DLL_COMM_LIB}" == 'yes' ]; then
	./cp_comm_lib.sh
	if [ ! -f DES-PWE_lib.tar.gz ]; then
		echo "压缩包: DES-PWE_lib.tar.gz 不存在, 动态链接库打包失败 !"
		exit 1
	fi
	mv DES-PWE_lib.tar.gz ../
fi

cd ..
Name=slog_all
TarF=${Name}.tar
TarP=$(pwd)/${TarF}
BackupDir=release_pack

# check 下所有服务代码是否编译成功
function check_module()
{
	mname=$1
	if [ ! -f ${mname}/${mname} ]; then
		echo "check file: ${mname}/${mname} failed !"
		rm -f ${TarP}
		exit 2
	fi
}
check_module slog_config
check_module slog_mtreport_client 
check_module slog_write 
check_module slog_deal_warn 
check_module slog_mtreport_server 
check_module slog_check_warn 
check_module slog_memcached 
check_module slog_server 
check_module slog_client 
check_module slog_monitor_server 

# check 下所有 cgi 是否编译成功
function check_cgi()
{
	mname=$1
	if [ ! -f cgi_fcgi/${mname} ]; then
		echo "check file: cgi_fcgi/${mname} failed !"
		rm -f ${TarP}
		exit 3
	fi
}
check_cgi mt_slog_machine
check_cgi mt_slog_monitor 
check_cgi mt_slog_showview 
check_cgi mt_slog_attr 
check_cgi mt_slog_view 
check_cgi mt_slog_warn 
check_cgi slog_flogin 
check_cgi mt_slog_user 
check_cgi mt_slog 

cd slog_mtreport_client
./make_fabu.sh
if [ ! -f slog_mtreport_client.tar.gz ]; then
    echo "生成 agent 发布包错误！"
    exit 4;
fi
cp slog_mtreport_client.tar.gz ..
cd ..

cp tools_sh/local_install.sh .
cp tools_sh/uninstall_DES-PWE.sh .

tar cvf ${TarP} tools_sh/rm_zero.sh
tar rvf ${TarP} slog_mtreport_client.tar.gz
tar rvf ${TarP} tools_sh/check_proc_monitor.sh
tar rvf ${TarP} tools_sh/stop_all.sh
tar rvf ${TarP} tools_sh/add_crontab.sh
tar rvf ${TarP} tools_sh/start_comm.sh
tar rvf ${TarP} tools_sh/stop_comm.sh
tar rvf ${TarP} cgi_fcgi/* --exclude *.cpp --exclude Makefile --exclude cgi_debug.txt 
tar rvf ${TarP} db
tar rvf ${TarP} html 

tar rvf ${TarP} local_install.sh
tar rvf ${TarP} uninstall_DES-PWE.sh 
if [ -f DES-PWE_lib.tar.gz ]; then
	tar rvf ${TarP} DES-PWE_lib.tar.gz
fi

dirlist=`find . -maxdepth 1 -type d`
for dr in $dirlist
do 
        if [ -f $dr/$dr -a -f $dr/$dr.conf ] ; then
            if [ "$dr" == "./slog_mtreport_client" ]; then
                mv $dr/slog_mtreport_client.conf $dr/_slog_mtreport_client.conf
                cp $dr/fabu_mtreport_client.conf $dr/slog_mtreport_client.conf
            fi
            tar rvf ${TarP} $dr/*.sh
            tar rvf ${TarP} $dr/*.conf
            tar rvf ${TarP} $dr/$dr
            if [ "$dr" == "./slog_mtreport_client" ]; then
                mv $dr/_slog_mtreport_client.conf $dr/slog_mtreport_client.conf
            fi

            if [ "$dr" == "./slog_config" ]; then
                tar rvf ${TarP} $dr/ipinfo.tar.gz
            fi
        fi
done

echo "打包中, 请稍等..."
cd tools_sh
mkdir _tmp
mv ../${TarF} _tmp
cd _tmp
tar -xf ${TarF}
rm ${TarF}
tar -czf ${TarF}.gz *

cd ..
cp _tmp/${TarF}.gz .
rm -fr _tmp
if [ ! -d ${BackupDir} ]; then
	mkdir -p ${BackupDir}
fi
CurDate=`date "+%Y%m%d"`
cp ${TarF}.gz ${BackupDir}/${TarF}.gz.$CurDate
rm -f ../uninstall_DES-PWE.sh
rm -f ../local_install.sh
rm -f ../DES-PWE_lib.tar.gz
rm -f ../slog_mtreport_client.tar.gz
echo "打包完成, 压缩包为：slog_all.tar.gz" 

