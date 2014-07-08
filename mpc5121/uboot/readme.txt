20140708
i2c.c 
rm i2c ack

!!!!!!!!!!!!!! change uboot/board/ads5121/Makefile , add flash.o intel_flash.o !!!!!
flash001.c // evb ??? 
flash003v1.c // board/evb64260/flash.c works

uboot/board/ads5121/ads5121.c
uboot/include/configs/ads5121.h
uboot/cpu/mpc512x/start.S
uboot/cpu/mpc512x/serial.c
uboot/cpu/mpc512x/cpu_init.c
uboot/board/ads5121/flash.c
-rw-r--r-- 1 u5121 u5121 16492 2009-03-21 17:04 uboot/board/sbc8240/flash.c flash002org.c
uboot/board/evb64260/flash.c flash003org.c

uboot/lib_ppc/board.c add rtl8139
uboot/drivers/mtd/nand/fsl_nfc_nand.c // nand write size mismatch,
compile:2048 , reset config words:512
