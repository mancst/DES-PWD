#!/bin/bash
# 备份当前目录下的文件，调用脚本时当前目录为脚本所在目录
#
#  通过环境变量 file_name 传入要备份的文件名
#

if [ "$file_name" == '' ]; then
    exit 1
fi

MAXCOUNTBACK=10
CURDATE=`date '+%Y%m%d-%H%M%S'`
cp ${file_name} ${file_name}_bk-${CURDATE}
COUNT=`ls -lt ${file_name}_bk*|wc -l`
if [ $COUNT -lt $MAXCOUNTBACK ]; then
    exit
fi
ls -lrt ${file_name}_bk* |awk '{if(NR==1) print $9}' |xargs rm -f {} \;

