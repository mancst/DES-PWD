#!/bin/bash

mid=`pgrep -f mysqld`
if [ $? -eq 0 -a "$mid" != '' ]; then
        kill $mid
fi

hid=`pgrep -f httpd-prefork`
if [ $? -eq 0 -a "$hid" != '' ]; then
        apachectl stop 
fi

cd slog_mtreport_client
./stop_plugin.sh
cd -

cd tools_sh
./stop_all.sh
cd ..

