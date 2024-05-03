#!/bin/bash

function start_proc()
{
	if [ $# -ne 1 ]; then 
		echo "use start_proc proc";
		exit -1
	fi

	export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:../tools_sh/DES-PWE_lib
	./$1
	
	pid=`ps -ef |grep $1$|grep -v tail|grep -v grep|awk '{print $2;}'`
	if [ -z "$pid" ]; then
	        echo "start failed !"
	        exit
	fi
	echo "$1 pid: $pid"
	rm _manual_stop_ > /dev/null 2>&1
}

