#!/bin/bash

rsync -r --progress install/usr/ /usr
rsync -r --progress install/etc/ /etc


