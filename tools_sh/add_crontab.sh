#!/bin/bash

crontab -l > _add_DES-PWE_proc_check 
cat _add_DES-PWE_proc_check |grep -v "^#" |grep "check_proc_monitor.sh" > /dev/null 2>&1
if [ $? -eq 0 ]; then
	rm _add_DES-PWE_proc_check 
	echo "already add"
	exit 0
fi

curpwd=`pwd`
echo "*/1 * * * * ${curpwd}/check_proc_monitor.sh > /dev/null 2>&1" >> _add_DES-PWE_proc_check 

crontab _add_DES-PWE_proc_check 
if [ $? -eq 0 ]; then
	rm _add_DES-PWE_proc_check 
	echo "add crontab ok"
	exit 0
fi

echo "add to crontab failed"
rm _add_DES-PWE_proc_check 
exit 1

