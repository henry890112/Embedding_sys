#!/bin/sh

set -x
# set -e

sudo rmmod -f mydev
sudo insmod mydev.ko

sudo mknod /dev/mydev c 456 0

./writer MYNAMEISHENRY &
./reader 127.0.0.1 12345 /dev/mydev
