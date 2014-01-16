/*
 * Copyright (C) 2007-2009 Freescale Semiconductor, Inc. All rights reserved.
 *
 * Author: John Rigby, <jrigby@freescale.com>, Thur Mar 29 2007
 *
 * Description:
 * MPC5121 ADS board setup
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/of_platform.h>

#include <asm/machdep.h>
#include <asm/ipic.h>
#include <asm/prom.h>
#include <asm/time.h>

#include <asm/mpc512x.h>
#include <sysdev/fsl_soc.h>

#include <linux/bootmem.h>
#include <asm/rheap.h>

#include "mpc512x.h"

#ifdef DEBUG
#define DPRINTK(fmt, args...) printk("%s: " fmt,__FUNCTION__,## args)
#else
#define DPRINTK(fmt, args...)
#endif

static u32 get_busfreq(void)
{
	struct device_node *node;

	u32 fs_busfreq=0;
	node = of_find_node_by_type(NULL, "cpu");
	if (node) {
		unsigned int size;
		const unsigned int *prop = of_get_property(node,"bus-frequency", &size);
		if (prop)
		fs_busfreq = *prop;
		of_node_put(node);
	};
	return fs_busfreq;
}

#ifdef CONFIG_FB_FSL_DIU

static rh_block_t diu_rh_block[16];
static rh_info_t diu_rh_info;

static unsigned long diu_size = 1280 * 1024 * 4; /* One 1280x1024 buffer */
static void *diu_mem;

unsigned int platform_get_pixel_format(unsigned int bits_per_pixel,
					int monitor_port,
					char byte_flip)
{
	unsigned int pix_fmt;

	if (bits_per_pixel == 32) {
		if (byte_flip)
			pix_fmt = 0x88883316;
		else
			pix_fmt = 0x88883306;
	} else if (bits_per_pixel == 24) {
		pix_fmt = 0x88082219;
	} else if (bits_per_pixel == 16) {
		if (byte_flip)
			pix_fmt = 0x65053118;
		else
			pix_fmt = 0x65052908;
	} else /* bits_per_pixel == 8, need to enable pallete table */
		pix_fmt = 0x00000400;

	return pix_fmt;
}
EXPORT_SYMBOL(platform_get_pixel_format);

void platform_set_gamma_table(int monitor_port, char *gamma_table_base)
{
}
EXPORT_SYMBOL(platform_set_gamma_table);

void platform_set_monitor_port(int monitor_port)
{
}
EXPORT_SYMBOL(platform_set_monitor_port);

void platform_set_pixel_clock(unsigned int pixclock)
{
	u32 * clkdvdr, temp;
	/* variables for pixel clock calcs */
	unsigned long  bestval, bestfreq, speed_ccb, minpixclock, maxpixclock, pixval;
	long err;
	int i;

	clkdvdr = (u32 *)ioremap(get_immrbase() + 0xf0c, sizeof(u32));

	/* Pixel Clock configuration */
	DPRINTK("DIU: Bus Frequency = %d\n",get_busfreq());
	speed_ccb = get_busfreq() * 4;

	/* Calculate the pixel clock with the smallest error */
	/* calculate the following in steps to avoid overflow */
	DPRINTK("DIU pixclock in ps - %d\n",pixclock);
	temp = 1;
	temp *= 1000000000;
	temp /= pixclock;
	temp *= 1000;
	pixclock = temp;
	DPRINTK("DIU pixclock freq - %lu\n",pixclock);

	temp *= 5;
	temp /= 100;  /* pixclock * 0.05 */
	DPRINTK("deviation = %d\n", temp);
	minpixclock = pixclock - temp;
	maxpixclock = pixclock + temp;
	DPRINTK("DIU minpixclock - %lu\n", minpixclock);
	DPRINTK("DIU maxpixclock - %lu\n", maxpixclock);
	pixval = speed_ccb/pixclock;
	DPRINTK("DIU pixval = %lu\n",pixval);

	err = 100000000;
	bestval = pixval;
	DPRINTK("DIU bestval = %lu\n", bestval);

	bestfreq = 0;
	for (i = -1; i <= 1; i++) {
		temp = speed_ccb / (pixval+i);
		DPRINTK("DIU test pixval i= %d, pixval=%lu, temp freq. = %u\n",i,pixval,temp);
		if ((temp < minpixclock) || (temp > maxpixclock))
			DPRINTK("DIU exceeds monitor range (%lu to %lu)\n",
				minpixclock,maxpixclock);
		else if (abs(temp - pixclock) < err) {
		  DPRINTK("Entered the else if block %d\n", i);
			err = abs(temp - pixclock);
			bestval = pixval+i;
			bestfreq = temp;
		}
	}

	DPRINTK("DIU chose = %lx\n", bestval);
	DPRINTK("DIU error = %ld\n NomPixClk ", err);
	DPRINTK("DIU: Best Freq = %lx\n",bestfreq);
	/* Modify PXCLK in GUTS CLKDVDR */
	DPRINTK("DIU: Current value of CLKDVDR = 0x%08x\n",(*clkdvdr));
	temp = (* clkdvdr) & 0xffffff00;
	* clkdvdr = temp | (bestval & 0xFF);
	DPRINTK("DIU: Modified value of CLKDVDR = 0x%08x\n",(*clkdvdr));
}
EXPORT_SYMBOL(platform_set_pixel_clock);

ssize_t platform_show_monitor_port(int monitor_port, char * buf)
{
	return snprintf(buf, PAGE_SIZE, "0 - 5121ads DVI & LCD\n");
}
EXPORT_SYMBOL(platform_show_monitor_port);

int platform_set_sysfs_monitor_port(int val)
{
	return 0;
}
EXPORT_SYMBOL(platform_set_sysfs_monitor_port);

static void __init preallocate_diu_videomemory(void)
{
	printk(KERN_INFO "%s: diu_size=%lu\n", __FUNCTION__, diu_size);

	diu_mem = __alloc_bootmem(diu_size, 8, 0);
	if (!diu_mem) {
		printk(KERN_ERR "fsl-diu: cannot allocate %lu bytes\n",
			diu_size);
		return;
	}

	printk(KERN_INFO "%s: diu_mem=%p\n", __FUNCTION__, diu_mem);

	rh_init(&diu_rh_info, 4096, ARRAY_SIZE(diu_rh_block), diu_rh_block);
	rh_attach_region(&diu_rh_info, (unsigned long) diu_mem, diu_size);
}

void *fsl_diu_alloc(unsigned long size, unsigned long *phys)
{
	 void *virt;

	 printk(KERN_DEBUG "%s: size=%lu\n", __FUNCTION__, size);
	 if (!diu_mem) {
		printk(KERN_INFO "%s: no diu_mem\n", __FUNCTION__);
		return NULL;
	 }

	 virt = (void *) rh_alloc(&diu_rh_info, size, "DIU");
	 if (virt)
		*phys = virt_to_bus(virt);

	 printk(KERN_DEBUG "%s:%u rh virt=%p phys=%lx\n",
			 __FUNCTION__, __LINE__, virt, *phys);

	 return virt;
}
EXPORT_SYMBOL(fsl_diu_alloc);

void fsl_diu_free(void *p, unsigned long size)
{
	printk(KERN_DEBUG "%s: p=%p size=%lu\n", __FUNCTION__, p, size);

	if (!p)
		return;

	if ((p >= diu_mem) && (p < (diu_mem + diu_size))) {
		printk(KERN_DEBUG "%s:%u rh\n", __FUNCTION__, __LINE__);
		rh_free(&diu_rh_info, (unsigned long) p);
	} else {
		printk(KERN_DEBUG "%s:%u dma\n", __FUNCTION__, __LINE__);
		dma_free_coherent(0, size, p, 0);
	}
}
EXPORT_SYMBOL(fsl_diu_free);

static int __init early_parse_diufb(char *p)
{
	if (!p)
		return 1;

	diu_size = _ALIGN_UP(memparse(p, &p), 8);

	printk(KERN_INFO "%s: diu_size=%lu\n", __FUNCTION__, diu_size);

	return 0;
}
early_param("diufb", early_parse_diufb);

#else

#define preallocate_diu_videomemory() do { } while (0)

#endif

#if defined(CONFIG_VIDEO_VIU) || defined(CONFIG_VIDEO_VIU_MODULE)
enum viu_io {
	VIU_DATA0   = 0x12c,
	VIU_DATA1   = 0x130,
	VIU_DATA2   = 0x134,
	VIU_DATA3   = 0x13c,
	VIU_DATA4   = 0x140,
	VIU_DATA5   = 0x144,
	VIU_DATA6   = 0x164,
	VIU_DATA7   = 0x154,
	VIU_DATA8   = 0x168,
	VIU_DATA9   = 0x16c,
	VIU_PIX_CLK = 0x160,
};

void fsl_viu_init_io(void)
{
	struct device_node *np;
	struct resource r;
	u8 __iomem *iobase;

	np = of_find_compatible_node(NULL, NULL, "fsl,mpc5121-ioctl");
	if (!np) {
		DPRINTK("Err: can't find device node 'mpc5121-ioctl'\n");
		return;
	}

	of_address_to_resource(np, 0, &r);
	of_node_put(np);

	iobase = (u8 *)ioremap(r.start, r.end - r.start + 1);
	out_be32((u32 *)(iobase + VIU_DATA0), 0x81);
	out_be32((u32 *)(iobase + VIU_DATA1), 0x81);
	out_be32((u32 *)(iobase + VIU_DATA2), 0x81);
	out_be32((u32 *)(iobase + VIU_DATA3), 0x81);
	out_be32((u32 *)(iobase + VIU_DATA4), 0x81);
	out_be32((u32 *)(iobase + VIU_DATA5), 0x81);
	out_be32((u32 *)(iobase + VIU_DATA6), 0x81);
	out_be32((u32 *)(iobase + VIU_DATA7), 0x81);
	out_be32((u32 *)(iobase + VIU_DATA8), 0x81);
	out_be32((u32 *)(iobase + VIU_DATA9), 0x81);
	out_be32((u32 *)(iobase + VIU_PIX_CLK), 0x81);

	iounmap(iobase);
}
EXPORT_SYMBOL(fsl_viu_init_io);
#endif

/**
 * 	mpc512x_find_ips_freq - Find the IPS bus frequency for a device
 * 	@node:	device node
 *
 * 	Returns IPS bus frequency, or 0 if the bus frequency cannot be found.
 */
unsigned long
mpc512x_find_ips_freq(struct device_node *node)
{
	struct device_node *np;
	const unsigned int *p_ips_freq = NULL;

	of_node_get(node);
	while (node) {
		p_ips_freq = of_get_property(node, "bus-frequency", NULL);
		if (p_ips_freq)
			break;

		np = of_get_parent(node);
		of_node_put(node);
		node = np;
	}
	if (node)
		of_node_put(node);

	return p_ips_freq ? *p_ips_freq : 0;
}
EXPORT_SYMBOL(mpc512x_find_ips_freq);

#define DEFAULT_FIFO_SIZE 16

static unsigned int get_fifo_size(struct device_node *np, int psc_num, char *fifo_name)
{
	const unsigned int *fp;

	fp = of_get_property(np, fifo_name, NULL);
	if (fp)
		return *fp;
	printk(KERN_WARNING "no %s property for psc%d defaulting to %d\n",
		fifo_name, psc_num, DEFAULT_FIFO_SIZE);
	return DEFAULT_FIFO_SIZE;
}

/*
 * If the kernel is using a different PSC for the console than
 * u-boot then that psc's clock will not be enabled.
 * The clock driver runs after console_init so turn on
 * the clock here.
 */
static void __init mpc5121_psc_lowlevel_clock_init(void)
{
	struct device_node *np;
	const u32 *cell_index;
	void __iomem *clockctl;

	np = of_find_compatible_node(NULL, NULL, "fsl,mpc5121-clock");
	clockctl = of_iomap(np, 0);
	of_node_put(np);

	if (clockctl) 
		for_each_compatible_node(np, NULL, "fsl,mpc5121-psc") {
			cell_index = of_get_property(np, "cell-index", NULL);
			if (cell_index) {
				setbits32(clockctl+4, 0x08000000 >> *cell_index);
			}
		}
	iounmap(clockctl);
}

/*
 * Disable any unused serial transceivers so they don't interfere with
 * spi or what ever else they might be used for.
 */
static void mpc5121ads_psc_disable_unused_serial(void)
{
	struct device_node *np;
	const u32 *cell_index;

	for_each_compatible_node(np, NULL, "fsl,mpc5121-psc") {
		cell_index = of_get_property(np, "cell-index", NULL);
		if (cell_index) {
			/*
			 * if not uart then then disable transceiver
			 */
			if (!of_device_is_compatible(np,
				    "fsl,mpc5121-psc-uart")) {
				if (*cell_index == 3) {
					mpc5121_ads_cpld_uart_foff(0);
				} else if (*cell_index == 4) {
					mpc5121_ads_cpld_uart_foff(1);
				}
			}
		}
	}
}

static int psc_fifo_base[12];
static void mpc5121_psc_fifo_init(char *name)
{
	struct device_node *np;
	const u32 *cell_index;
	int fifobase = 0; /* current fifo address in 32 bit words */
	char *default_psc = "fsl,mpc5121-psc";
	char *psc_name;

	if (name)
		psc_name = name;
	else
		psc_name = default_psc;

	for_each_compatible_node(np, NULL, psc_name) {
		cell_index = of_get_property(np, "cell-index", NULL);
		if (cell_index) {
			int psc_num = *cell_index;
			unsigned int tx_fifo_size;
			unsigned int rx_fifo_size;
			void __iomem *psc;

			tx_fifo_size = get_fifo_size(np, psc_num, "tx-fifo-size");
			rx_fifo_size = get_fifo_size(np, psc_num, "rx-fifo-size");

			/* size in register is in 4 byte words */
			tx_fifo_size /= 4;
			rx_fifo_size /= 4;

			psc = of_iomap(np, 0);

			if(strcmp(psc_name, default_psc)) {
				fifobase = psc_fifo_base[psc_num];
				/* tx fifo size register is at 0x9c and rx at 0xdc */
				out_be32(psc + 0x9c, (fifobase << 16) | tx_fifo_size);
				fifobase += tx_fifo_size;
				out_be32(psc + 0xdc, (fifobase << 16) | rx_fifo_size);
			} else {
				psc_fifo_base[psc_num] = fifobase;
				/* tx fifo size register is at 0x9c and rx at 0xdc */
				out_be32(psc + 0x9c, (fifobase << 16) | tx_fifo_size);
				fifobase += tx_fifo_size;
				out_be32(psc + 0xdc, (fifobase << 16) | rx_fifo_size);
				fifobase += rx_fifo_size;
			}

			/* reset and enable the slices */
			out_be32(psc + 0x80, 0x80);
			out_be32(psc + 0x80, 0x01);
			out_be32(psc + 0xc0, 0x80);
			out_be32(psc + 0xc0, 0x01);

			iounmap(psc);
		}
	}
}


#define IO_PSC_0_0_ADDR_OFFSET	0x20c
#define IO_PSC_PIN_SIZE		0x14
#define IO_PSC_PIN_OFFSET(x)	(IO_PSC_0_0_ADDR_OFFSET + IO_PSC_PIN_SIZE * (x))

static void mpc5121_psc_iopad_init(void __iomem *ioctl, char *name)
{
	struct device_node *np;
	const u32 *cell_index;
	char *default_psc = "fsl,mpc5121-psc";
	char *psc_name;

	if (name)
		psc_name = name;
	else
		psc_name = default_psc;

	for_each_compatible_node(np, NULL, psc_name) {
		cell_index = of_get_property(np, "cell-index", NULL);
		if (cell_index) {
			u32 __iomem *pscioctl;
			int psc_num = *cell_index;

			pscioctl = ioctl + IO_PSC_PIN_OFFSET(psc_num);
			out_be32(pscioctl++, 0x07);	/* PSCn_0, STD_ST */
			out_be32(pscioctl++, 0x03);	/* PSCn_1, STD */
			out_be32(pscioctl++, 0x03);	/* PSCn_2, STD */
			out_be32(pscioctl++, 0x03);	/* PSCn_3, STD */
			out_be32(pscioctl++, 0x03);	/* PSCn_4, STD */
		}
	}
}

static void __init mpc5121_can_iopad_init(void __iomem *ioctl)
{
	struct device_node *np;
	const u32 *cell_index;

	for_each_compatible_node(np, NULL, "fsl,mpc5121-mscan") {
		cell_index = of_get_property(np, "cell-index", NULL);
		if (cell_index) {
			u32 __iomem *canioctl;
			int can_num = *cell_index;

			/*
			 * Config can Tx pin
			 * 0x1f8 is offset to CAN0_Tx
			 */
			canioctl = ioctl + 0x1f8 + 4 * can_num;
			out_be32(canioctl, 0x03);
		}
	}
}

void mpc5121ads_diu_io_pm_restore(void)
{
	struct device_node *np;

#define DIU_CLK		0x284
#define DIU_HSYN	0x288
#define DIU_IO_OFFSET	0x294
#define DIU_IO_SIZE	0x68
	/*
	 * io pad config
	 */
	np = of_find_compatible_node(NULL, NULL, "fsl,mpc5121-ioctl");
	if (np) {
		void __iomem *ioctl = of_iomap(np, 0);
		int i, count = DIU_IO_SIZE / 4;
		u32 *reg;

		reg = ioctl + DIU_CLK;
		*reg = 0x107;
		reg = ioctl + DIU_HSYN;
		*reg = 0x103;
		reg = ioctl + DIU_IO_OFFSET;
		for (i = 0; i < count; i++)
			*reg++ = 0x103;

		of_node_put(np);
		iounmap(ioctl);
	}
}
EXPORT_SYMBOL(mpc5121ads_diu_io_pm_restore);

void mpc5121ads_ac97_pm_restore(void)
{
	struct device_node *np;

	/*
	 * io pad config
	 */
	np = of_find_compatible_node(NULL, NULL, "fsl,mpc5121-ioctl");
	if (np) {
		void __iomem *ioctl = of_iomap(np, 0);

		mpc5121_psc_iopad_init(ioctl, "fsl,mpc5121-psc-ac97");

		of_node_put(np);
		iounmap(ioctl);
	}

	mpc5121_psc_fifo_init("fsl,mpc5121-psc-ac97");
}
EXPORT_SYMBOL(mpc5121ads_ac97_pm_restore);

static void __init mpc5121ads_board_setup(void)
{
	struct device_node *np;
	void __iomem *i2cctl;

	/*
	 * cpld regs are needed early
	 */
	mpc5121_ads_cpld_map();
	/*
	 * io pad config
	 */
	np = of_find_compatible_node(NULL, NULL, "fsl,mpc5121-ioctl");
	if (np) {
		void __iomem *ioctl = of_iomap(np, 0);

		mpc5121_psc_iopad_init(ioctl, NULL);
		mpc5121_can_iopad_init(ioctl);

		of_node_put(np);
		iounmap(ioctl);
	}

	mpc5121_psc_lowlevel_clock_init();
	mpc5121_psc_fifo_init(NULL);
	mpc5121ads_psc_disable_unused_serial();

	/*
	 * turn on i2c interrupts
	 */
	np = of_find_compatible_node(NULL, NULL, "fsl,mpc5121-i2c-ctrl");
	if (np) {
		i2cctl = of_iomap(np, 0);
		of_node_put(np);
		if (i2cctl) {
			out_be32(i2cctl, 0x15000000);
			iounmap(i2cctl);
		}
	}
}

static void __init mpc5121_ads_setup_arch(void)
{
#ifdef CONFIG_PCI
	struct device_node *np;
#endif
	printk(KERN_INFO "MPC5121 ADS board from Freescale Semiconductor\n");

	preallocate_diu_videomemory();
	mpc5121ads_board_setup();
 
#ifdef CONFIG_PM
	#ifdef CONFIG_MPC5121_ADS_HIB
		ads5121_pm_init();
	#else
		mpc512x_pm_init();
	#endif
#endif

#ifdef CONFIG_PCI
	for_each_compatible_node(np, "pci", "fsl,mpc5121-pci")
		mpc512x_add_bridge(np);
#endif
}

static struct of_device_id __initdata of_bus_ids[] = {
	{ .name = "soc", },
	{ .name = "localbus", },
	{ .compatible = "fsl,mpc5121-nfc", },
	{ .compatible = "fsl,mpc5121rev2-nfc", },
	{ .compatible = "fsl,mpc5121-mbx", },
	{},
};

static void __init mpc5121_ads_declare_of_platform_devices(void)
{
	if (of_platform_bus_probe(NULL, of_bus_ids, NULL))
		printk(KERN_ERR __FILE__ ": "
			"Error while probing of_platform bus\n");
}

static void __init mpc5121_ads_init_IRQ(void)
{
	struct device_node *np;

	np = of_find_compatible_node(NULL, NULL, "fsl,ipic");
	if (!np)
		return;

	ipic_init(np, 0);
	of_node_put(np);

	/*
	 * Initialize the default interrupt mapping priorities,
	 * in case the boot rom changed something on us.
	 */
	ipic_set_default_priority();

	/*
	 * Intialize the cpld interrupt host
	 */
#if defined(CONFIG_PCI) || defined(CONFIG_TOUCHSCREEN_ADS7846)
	mpc5121_ads_cpld_pic_init();
#endif
}

/*
 * Called very early, MMU is off, device-tree isn't unflattened
 */
static int __init mpc5121_ads_probe(void)
{
	unsigned long root = of_get_flat_dt_root();

	return of_flat_dt_is_compatible(root, "fsl,mpc5121ads");
}

define_machine(mpc5121_ads) {
	.name			= "MPC5121 ADS",
	.probe			= mpc5121_ads_probe,
	.setup_arch		= mpc5121_ads_setup_arch,
	.init			= mpc5121_ads_declare_of_platform_devices,
	.init_IRQ		= mpc5121_ads_init_IRQ,
	.get_irq		= ipic_get_irq,
	.calibrate_decr		= generic_calibrate_decr,
};
