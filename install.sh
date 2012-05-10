#!/bin/bash

dir=`dirname $0`
cp $dir/pd-external/*.pd_linux $dir/install/usr/local/lib/addacs/
sudo rsync -r --progress $dir/install/usr/ /usr
sudo rsync -r --progress $dir/install/etc/ /etc



