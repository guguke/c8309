#!/bin/sh

#echo "export PATH=$PATH:/opt/eldk-5.4/powerpc-e500v2/sysroots/i686-eldk-linux/usr/bin/ppce500v2-linux-gnuspe/"
#export PATH=$PATH:/opt/eldk-5.4/powerpc-e500v2/sysroots/i686-eldk-linux/usr/bin/ppce500v2-linux-gnuspe/
#echo "CROSS_COMPILE=powerpc-e500v2-"
#export CROSS_COMPILE=powerpc-e500v2-
export PATH=$PATH:/opt/freescale/usr/local/gcc-4.3.74-eglibc-2.8.74-dp-2/powerpc-none-linux-gnuspe/bin/:/opt/freescale/ltib/usr/bin/
export CROSS_COMPILE=powerpc-none-linux-gnuspe-
export ARCH=powerpc

#make distclean
make menuconfig
#make cuImage.p1020rdb
