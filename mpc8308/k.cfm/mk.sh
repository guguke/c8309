#!/bin/sh

#export PATH=$PATH:/opt/freescale_mpc8308/usr/local/gcc-4.1.78-eglibc-2.5.78-1/powerpc-e300c3-linux-gnu/bin/
export PATH=$PATH:/opt/freescale/usr/local/gcc-4.1.78-eglibc-2.5.78-1/powerpc-e300c3-linux-gnu/bin/:/opt/freescale/ltib/usr/bin/
# /opt/freescale/ltib/usr/bin     ==> mkimage

export CROSS_COMPILE=powerpc-e300c3-linux-gnu-
export ARCH=powerpc

#make distclean
#make menuconfig
make cuImage.mpc8308erdb
