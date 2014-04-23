/*
 * arch/powerpc/platforms/83xx/mpc8309_som.c
 *
 * Description: MPC8309 SOM board specific routines.
 *
 * Copyright (C) Freescale Semiconductor, Inc. 2011. All rights reserved.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/pci.h>
#include <linux/of_platform.h>
#include <linux/fsl_devices.h>
#include <linux/spi/spi.h>
#include <linux/i2c.h>
#include <asm/time.h>
#include <asm/ipic.h>
#include <asm/udbg.h>
#include <asm/qe.h>
#include <asm/qe_ic.h>
#include <sysdev/fsl_pci.h>
#include <sysdev/fsl_soc.h>
#include "mpc83xx.h"

/*
 * Setup the architecture
 */
static void __init mpc8309_som_setup_arch(void)
{
	void __iomem *immap;
#ifdef CONFIG_PCI
	struct device_node *np;
#endif
	if (ppc_md.progress)
		ppc_md.progress("mpc8309_som_setup_arch()", 0);

#ifdef CONFIG_PCI
	for_each_compatible_node(np, "pci", "fsl,mpc8349-pci")
		mpc83xx_add_bridge(np);
#endif
#ifdef CONFIG_USB_SUPPORT
	mpc8309_usb_cfg();
#endif

	immap = ioremap(get_immrbase(), 0x1000);

	/* set the I/O configuration for I2C2 */
	clrsetbits_be32(immap + MPC83XX_SICRL_OFFS, MPC8309_SICRL_I2C2_MASK,
			MPC8309_SICRL_I2C2);

#ifdef CONFIG_FLEXCAN_MPC830X
	/* Set I/O configuration for FlexCAN - CAN1 */
	clrsetbits_be32(immap + MPC83XX_SICRL_OFFS, MPC8309_SICRL_CAN1_MASK,
			MPC8309_SICRL_CAN1);
#if 1
	clrsetbits_be32(immap + MPC83XX_SICRL_OFFS, MPC8309_SICRL_CAN2_MASK,
			MPC8309_SICRL_CAN2);
	clrsetbits_be32(immap + MPC83XX_SICRL_OFFS, MPC8309_SICRL_CAN3_MASK,
			MPC8309_SICRL_CAN3);
	clrsetbits_be32(immap + MPC83XX_SICRL_OFFS, MPC8309_SICRL_CAN4_MASK,
			MPC8309_SICRL_CAN4);   
#endif

	/* Set CAN access control register for normal supervisor mode */
	clrsetbits_be32(immap + MPC830X_CAN_CTRL_OFFS, MPC830X_CAN1_CTRL_MASK,
			MPC830X_CAN1_SUPV_MODE);
#if 1
	clrsetbits_be32(immap + MPC830X_CAN_CTRL_OFFS, MPC830X_CAN2_CTRL_MASK,
			MPC830X_CAN2_SUPV_MODE);
	clrsetbits_be32(immap + MPC830X_CAN_CTRL_OFFS, MPC830X_CAN3_CTRL_MASK,
			MPC830X_CAN3_SUPV_MODE);
	clrsetbits_be32(immap + MPC830X_CAN_CTRL_OFFS, MPC830X_CAN4_CTRL_MASK,
			MPC830X_CAN4_SUPV_MODE);
#endif
#endif

#ifdef CONFIG_QUICC_ENGINE

#ifdef CONFIG_UCC_TDM_IO
	/* set the I/O configuration for TDM2 for SLIC */
	clrsetbits_be32(immap + MPC83XX_SICRH_OFFS, MPC8309_SICRH_TDM2_MASK,
			MPC8309_SICRH_TDM2);
	/* set up BRG3 & BRG9 for SLIC */
	clrsetbits_be32(immap + MPC83XX_SICRH_OFFS, 0x000c0000, 0x000c0000);
#endif
#ifdef CONFIG_UCC_TDM_FRAMER_IO
	/* set the I/O configuration for TDM1 for T1/E1 framer */
	clrsetbits_be32(immap + MPC83XX_SICRH_OFFS, MPC8309_SICRH_TDM1_MASK,
			MPC8309_SICRH_TDM1);
#endif

	qe_reset();
#endif				/* CONFIG_QUICC_ENGINE */

	iounmap(immap);
}

static void __init mpc8309_som_init_IRQ(void)
{
	struct device_node *np;

	np = of_find_node_by_type(NULL, "ipic");
	if (!np)
		return;

	ipic_init(np, 0);

	/* Initialize the default interrupt mapping priorities,
	 * in case the boot rom changed something on us.
	 */
	ipic_set_default_priority();
	of_node_put(np);

#ifdef CONFIG_QUICC_ENGINE
	np = of_find_compatible_node(NULL, NULL, "fsl,qe-ic");
	if (!np) {
		np = of_find_node_by_type(NULL, "qeic");
		if (!np)
			return;
	}
	qe_ic_init(np, 0, qe_ic_cascade_low_ipic, qe_ic_cascade_high_ipic);
	of_node_put(np);
#endif				/* CONFIG_QUICC_ENGINE */
}

/*
 * Called very early, MMU is off, device-tree isn't unflattened
 */
static int __init mpc8309_som_probe(void)
{
	unsigned long root = of_get_flat_dt_root();

	return of_flat_dt_is_compatible(root, "fsl,mpc8309som");
}

static struct of_device_id __initdata of_bus_ids[] = {
	{ .compatible = "simple-bus" },
	{ .compatible = "fsl,qe" },
	{},
};

static int __init declare_of_platform_devices(void)
{
	of_platform_bus_probe(NULL, of_bus_ids, NULL);
	return 0;
}
machine_device_initcall(mpc8309_som, declare_of_platform_devices);

define_machine(mpc8309_som) {
	.name			= "MPC8309 SOM",
	.probe			= mpc8309_som_probe,
	.setup_arch		= mpc8309_som_setup_arch,
	.init_IRQ		= mpc8309_som_init_IRQ,
	.get_irq		= ipic_get_irq,
	.restart		= mpc83xx_restart,
	.time_init		= mpc83xx_time_init,
	.calibrate_decr		= generic_calibrate_decr,
	.progress		= udbg_progress,
};

/* I2C1 address for GPIO 4Bit expander and bit definition */
#define GPIO_4B_EXPND_I2C1_ADDR	0x41
#define SD_CAN_MUX_HIGH			0x01	/* 1 for CAN, 0 for SD */
#define UART_LBC_MUX_HIGH		0x02
#define BOOT_STATUS_GPIO_HIGH	0x04

/* I2C2 address for GPIO I/O expander1 and bits definition*/
#define GPIO_EXPND_I2C_ADDR		0x20
#define SPI_EEPROM_CS_HIGH		0x02
#define SPI_SLIC_CS_HIGH		0x01
#define GPIO_CAN_EN_HIGH		0x20
#define GPIO_CAN_STY_HIGH		0x40

/* I2C2 address for GPIO I/O expander2 and bits definition*/
#define GPIO_EXPND2_I2C_ADDR	0x21
#define USB_SLIC_SEL			0x40

static int enable;
static u8 expander1;
//static u8 gpio_4B_expander = 0xff;

/* Write to IO expander thru I2C interface */
int gpio_expander_i2c_write_byte(u8 devaddr, u8 regoffset, u8 value, u8 i2c_id)
{
	struct i2c_adapter *adap;
	int err;
	struct i2c_msg msg[1];
	unsigned char data[2];

	adap = i2c_get_adapter(i2c_id);
	if (!adap)
		return -ENODEV;
	msg->addr = devaddr;	/* I2C address of chip */
	msg->flags = 0;
	msg->len = 2;
	msg->buf = data;
	data[0] = regoffset;	/* register num */
	data[1] = value;		/* register data */
	err = i2c_transfer(adap, msg, 1);
	i2c_put_adapter(adap);
	if (err >= 0)
		return 0;
	return err;
}

/* Read from IO expander thru I2C interface */
int gpio_expander_i2c_read_byte(u8 devaddr, u8 regoffset, u8 *value, u8 i2c_id)
{
	struct i2c_adapter *adap;
	int err;
	struct i2c_msg msg[1];
	unsigned char data[2];

	adap = i2c_get_adapter(i2c_id);
	if (!adap)
		return -ENODEV;

	msg->addr = devaddr;	/* I2C address of chip */
	msg->flags = 0;
	msg->len = 1;
	msg->buf = data;
	data[0] = regoffset;	/* register num */
	err = i2c_transfer(adap, msg, 1);

	msg->addr = devaddr;	/* I2C address */
	msg->flags = I2C_M_RD;
	msg->len = 1;
	msg->buf = data;
	err = i2c_transfer(adap, msg, 1);
	 *value = data[0];
	i2c_put_adapter(adap);

	if (err >= 0)
		return 0;
	return err;
}

static int ioexpander_enable(void)
{
	u32 status = 0;
	u8 config_reg;

	/*Set up IOexpander configuration register for Input/Ouput via I2C2 */
	status = gpio_expander_i2c_read_byte(GPIO_EXPND_I2C_ADDR, 0x03,
					&config_reg, 1);
	if (status != 0) {
		printk(KERN_INFO "IO Expander value read error\n");
		return status;
	}
	/* Set the GPIO pin as output for slic_cs */
	config_reg &= 0xFE;

	/* set up GPIO for can_en & can_sty as output and can_err as input */
	config_reg &= ~0x60;
	config_reg |= 0x80;

	status = gpio_expander_i2c_write_byte(GPIO_EXPND_I2C_ADDR, 0x03,
					config_reg, 1);
	if (status != 0) {
		printk(KERN_INFO "IO Expander value write error\n");
		return status;
	}

	/* mux_ctl=0 eeprom_cs=0(inactive) slic_cs=1(inactive)
	   can_en=0(inactive) can_stby=0(inactive)*/
	expander1 = 0x01;
	status = gpio_expander_i2c_write_byte(GPIO_EXPND_I2C_ADDR, 0x01,
					expander1, 1);
	if (status != 0) {
		printk(KERN_INFO "IO Expander value write error\n");
		return status;
	}

	return status;
}

/*
   The Enable function for the CAN interface.
   on = 1 for enable the CAN interface
   on = 0 for disable the CAN inteface
*/
void mpc830x_CAN_control(bool on)
{
#if 0
	u32 status;
	u8 tmp;

	if (enable == 0) {
		ioexpander_enable();
		enable++;
	}
	tmp = expander1;
	if (on)
		tmp |= (GPIO_CAN_EN_HIGH | GPIO_CAN_STY_HIGH);
	else
		tmp &= ~(GPIO_CAN_EN_HIGH | GPIO_CAN_STY_HIGH);

	expander1 = tmp;
	status = gpio_expander_i2c_write_byte(GPIO_EXPND_I2C_ADDR, 0x01,
					tmp, 1);
	if (status != 0) {
		printk(KERN_INFO "IO Expander value write error\n");
		return;
	}
#endif
}
EXPORT_SYMBOL(mpc830x_CAN_control);

/*
   I/O configurations are muxed between SD and CAN.
   This routine will enable the mux control for CAN operation using I2C1
 */
void mpc830x_CAN_mux_enable(void)
{
#if 0
	u32 status;
	u8 tmp;

	/* Set can/LBC mux and boot_status as output */
	status = gpio_expander_i2c_read_byte(GPIO_4B_EXPND_I2C1_ADDR, 0x03,
					&tmp, 0);
	status = gpio_expander_i2c_write_byte(GPIO_4B_EXPND_I2C1_ADDR, 0x03,
					0xfa, 0);

	/* Set can/lbc mux as low and boot_status as high */
	gpio_4B_expander |= 0x04;
	gpio_4B_expander &= 0xfe;
	status = gpio_expander_i2c_write_byte(GPIO_4B_EXPND_I2C1_ADDR, 0x01,
					gpio_4B_expander, 0);
#endif
}
EXPORT_SYMBOL(mpc830x_CAN_mux_enable);

/*
   I/O configurations are muxed between SLIC and USB
   This routine will enable the mux control for SLIC operation using
   I2C2 EXPANDER2 P6.
 */
void mpc830x_SLIC_mux_enable(void)
{
	u32 status;
	u8 tmp;
	u8 config_reg;

	/* Read IOexpander2 configuration register */
	status = gpio_expander_i2c_read_byte(GPIO_EXPND2_I2C_ADDR, 0x03,
					&config_reg, 1);
	/* setup P6 as output */
	config_reg &= ~USB_SLIC_SEL;
	status = gpio_expander_i2c_write_byte(GPIO_EXPND2_I2C_ADDR, 0x03,
					config_reg, 1);

	/* Set USB_SLIC_SEL as low to enable */
	status = gpio_expander_i2c_read_byte(GPIO_EXPND2_I2C_ADDR, 0x01,
					&tmp, 1);
	tmp &= ~USB_SLIC_SEL;
	status = gpio_expander_i2c_write_byte(GPIO_EXPND2_I2C_ADDR, 0x01,
					tmp, 1);

}
EXPORT_SYMBOL(mpc830x_SLIC_mux_enable);

/*
 * Chipselect functions for MPC_SLIC_SPI_CS of I/O Expander-1
 * The chipselect which has to be asserted or deasserted.
 * on: True for setting high, False for setting low.
 */
void mpc830x_slic_spi_cs_control(struct spi_device *spi, bool on)
{
	u32 status;
	u8 tmp;

	if (enable == 0) {
		ioexpander_enable();
		enable++;
	}

	tmp = expander1;
	if (on)
		tmp |= SPI_SLIC_CS_HIGH;
	else
		tmp &= ~SPI_SLIC_CS_HIGH;

	expander1 = tmp;
	status = gpio_expander_i2c_write_byte(GPIO_EXPND_I2C_ADDR, 0x01, tmp, 1);
	if (status != 0) {
		printk(KERN_INFO "IO Expander value write error\n");
		return;
	}
}

static int __init of_fsl_spi_probe(char *type, char *compatible, u32 sysclk,
				   struct spi_board_info *board_infos,
				   unsigned int num_board_infos,
			       void (*cs_control)(struct spi_device *spi, bool on))
{
	struct device_node *np;
	unsigned int i = 0;

	for_each_compatible_node(np, type, compatible) {
		int ret;
		unsigned int j;
		const void *prop;
		struct resource res[2];
		struct platform_device *pdev;
		struct fsl_spi_platform_data pdata = {
			.cs_control = cs_control,
		};

		memset(res, 0, sizeof(res));

		pdata.sysclk = sysclk;

		prop = of_get_property(np, "reg", NULL);
		if (!prop)
			goto err;
		pdata.bus_num = *(u32 *)prop;

		prop = of_get_property(np, "cell-index", NULL);
		if (prop)
			i = *(u32 *)prop;

		prop = of_get_property(np, "mode", NULL);
		if (prop && !strcmp(prop, "cpu-qe"))
			pdata.flags = SPI_QE_CPU_MODE;

		for (j = 0; j < num_board_infos; j++) {
			if (board_infos[j].bus_num == pdata.bus_num)
				pdata.max_chipselect++;
		}

		if (!pdata.max_chipselect)
			continue;

		ret = of_address_to_resource(np, 0, &res[0]);
		if (ret)
			goto err;

		ret = of_irq_to_resource(np, 0, &res[1]);
		if (ret == NO_IRQ)
			goto err;

		pdev = platform_device_alloc("mpc8xxx_spi", i);
		if (!pdev)
			goto err;

		ret = platform_device_add_data(pdev, &pdata, sizeof(pdata));
		if (ret)
			goto unreg;

		ret = platform_device_add_resources(pdev, res,
						    ARRAY_SIZE(res));
		if (ret)
			goto unreg;

		ret = platform_device_add(pdev);
		if (ret)
			goto unreg;

		goto next;
unreg:
		platform_device_del(pdev);
err:
		pr_err("%s: registration failed\n", np->full_name);
next:
		i++;
	}

	return i;
}

int __init fsl_spi_init(struct spi_board_info *board_infos,
			unsigned int num_board_infos,
			void (*cs_control)(struct spi_device *dev, bool on))
{
	u32 sysclk = -1;
	int ret;

	if (sysclk == -1) {
		sysclk = fsl_get_sys_freq();
		if (sysclk == -1)
			return -ENODEV;
	}

	ret = of_fsl_spi_probe(NULL, "fsl,spi", sysclk, board_infos,
			       num_board_infos, cs_control);
	if (!ret)
		of_fsl_spi_probe("spi", "fsl_spi", sysclk, board_infos,
				 num_board_infos, cs_control);

	return spi_register_board_info(board_infos, num_board_infos);
}

/*
 * This structure gives the information of the SPI slave connected on the board.
 */
static struct spi_board_info mpc8309_slic_spi_boardinfo = {
	.modalias = "proslic",
	.bus_num = 0x7000,
	.chip_select = 0,
	.max_speed_hz = 8000000,
	.mode = SPI_CPHA | SPI_CPOL,
};

static int __init mpc8309_spi_init(void)
{
	return fsl_spi_init(&mpc8309_slic_spi_boardinfo, 1,
			mpc830x_slic_spi_cs_control);
}
machine_device_initcall(mpc8309_som, mpc8309_spi_init);

