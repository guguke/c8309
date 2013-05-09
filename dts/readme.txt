
2013.05.09
mi.dts  <==  long/mpc8309-v1-130130.dts
oi.dts  <==  org/mpc8309som.dts  (org) linux/arch/powerpc/boot/dts/mpc8309som.dts
indent mpc8309....dts -o oi.dts
indent mpc....-v1......dts -o mi.dts
diff -u oi.dts mi.dts > d.txt

