mpc8309 714
20140428
mpc8309som.dts : current
mpc8309som_v0.dts : ltib org    cp from git/serial/fre24m/
mpc8309-v1-130130p93.dts : dts: 28       cp from git/serial/fre24m/

2013.05.21
  arch/powerpc/kernel/legacy_serial.c
  drivers/serial/8250.c
2014.04.23
  mpc8309_som_gpio.c   <== mpc8309_som.c      
    add gpio0-5 multiplex config
2013.05.14
  mpc830x_drv.c
    irq share
2013.05.13
  mpc8309_som.c
    rm func CAN_control() and CAN_mux_enable()
2013.05.09
can bus setup

file arch/powerpc/platforms/83xx/mpc83xx.h
file arch/powerpc/platforms/83xx/mpc8309_som.c

linux/drivers/net/can/flexcan/mpc830x_drv.c
