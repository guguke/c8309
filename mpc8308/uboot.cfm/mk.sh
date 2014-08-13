#!/bin/sh

# export PATH=$PATH:/opt/freescale_mpc8308/usr/local/gcc-4.1.78-eglibc-2.5.78-1/powerpc-e300c3-linux-gnu/bin/
export PATH=$PATH:/opt/freescale/usr/local/gcc-4.1.78-eglibc-2.5.78-1/powerpc-e300c3-linux-gnu/bin/
export CROSS_COMPILE=powerpc-e300c3-linux-gnu-

make distclean
#make MPC8308ERDB_16bit_config
make MPC8308ERDB_config
#make MPC8308ERDB_NAND_config
#make MPC8308ERDB_NAND_ECC_OFF_config
make

