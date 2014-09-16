#!/bin/sh

echo "export PATH=$PATH:/opt/eldk-5.4/powerpc-e500v2/sysroots/i686-eldk-linux/usr/bin/ppce500v2-linux-gnuspe/"
export PATH=$PATH:/opt/eldk-5.4/powerpc-e500v2/sysroots/i686-eldk-linux/usr/bin/ppce500v2-linux-gnuspe/
echo "CROSS_COMPILE=powerpc-e500v2-"
export CROSS_COMPILE=powerpc-e500v2-
export ARCH=powerpc

#make distclean
make menuconfig
#make cuImage.p1020rdb
