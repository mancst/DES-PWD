#!/bin/bash

if [ ! -f "${conf_file}" ]; then
    echo "not find file:${conf_file}"
    exit 2
fi

fdirname=`dirname ${conf_file}`
cd $fdirname

# 插件目录下有上报配置的脚本则优先使用插件自己的脚本
if [ -x report_cfg.sh ]; then
    ./report_cfg.sh
    exit 0
fi

cfgs=`cat $conf_file|grep "^XRK_ENABLE_MOD_CFGS "|awk '{print $2}'`
inner_cfgs=`cat $conf_file|grep "^XRK_ALL_INNER_CFGS "|awk '{print $2}'`
IFSBAK=$IFS
IFS=','
last_mod_time=`stat -c %Y $conf_file`
echo "DES-PWE_cfgs:${last_mod_time}" > _tmp_cfgs
for item in $cfgs
do
    end_prex=${item:0:7}
    if [ "$end_prex" == 'xrk_end' ]; then break; fi
    cfg_info=`cat $conf_file|grep "^$item "`
    echo ";" >> _tmp_cfgs
    echo $cfg_info >> _tmp_cfgs
done
IFS=$IFSBAK

all_cfgs=`cat $conf_file|grep -v ^# |awk '{if(NF == 2) print $1; }'`
for item2 in $all_cfgs
do
    echo $inner_cfgs|grep "$item2," > /dev/null 2>&1
    if [ $? -eq 0 ]; then
        continue;
    fi
    echo $cfgs|grep "$item2," > /dev/null 2>&1
    if [ $? -eq 0 ]; then
        continue;
    fi

    cfg_info2=`cat $conf_file|grep "^$item2 "`
    echo ";" >> _tmp_cfgs
    echo $cfg_info2 >> _tmp_cfgs
done

sed -i 's/\r//g' _tmp_cfgs
cat _tmp_cfgs|xargs echo -e |sed 's/ ; /;/g'

