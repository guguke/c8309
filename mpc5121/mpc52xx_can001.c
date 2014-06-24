/*
 * DESCRIPTION:
 *  CAN bus driver for the Freescale MPC52xx embedded CPU.
 *
 * AUTHOR:
 *  Andrey Volkov <avolkov@varma-el.com>
 *
 * COPYRIGHT:
 *  2004-2005, Varma Electronics Oy
 *
 * LICENCE:
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * HISTORY:
 * 	 2008-02-26 Add support for MPC512x
 * 	 		Hongjun, Chen <hong-jun.chen@freescale.com>
 *	 2005-02-03 created
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/netdevice.h>
#include <linux/can.h>
#include <linux/can/dev.h>
#include <asm/io.h>
#include <asm/of_platform.h>
#include <asm/mpc512x.h>

#include "mscan.h"

#include <linux/can/version.h>	/* for RCSID. Removed by mkpatch script */

RCSID("$Id$");

#define PDEV_MAX 4

struct platform_device *pdev[PDEV_MAX];

static int __devinit mpc52xx_can_probe(struct platform_device *pdev)
{
	struct resource *mem;
	struct net_device *dev;
	struct mscan_platform_data *pdata = pdev->dev.platform_data;
	struct can_priv *can;
	u32 mem_size;
	int ret = -ENODEV;

	if (!pdata)
		return ret;

	dev = alloc_mscandev();
	if (!dev)
		return -ENOMEM;
	can = netdev_priv(dev);

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	dev->irq = platform_get_irq(pdev, 0);
	if (!mem || !dev->irq)
		goto req_error;

	mem_size = mem->end - mem->start + 1;
	if (!request_mem_region(mem->start, mem_size, pdev->dev.driver->name)) {
		dev_err(&pdev->dev, "resource unavailable\n");
		goto req_error;
	}

	SET_NETDEV_DEV(dev, &pdev->dev);

	dev->base_addr = (unsigned long)ioremap_nocache(mem->start, mem_size);

	if (!dev->base_addr) {
		dev_err(&pdev->dev, "failed to map can port\n");
		ret = -ENOMEM;
		goto fail_map;
	}

	if (pdata->cpu_type == MPC512x_MSCAN) {
		struct clk *mscan_clk, *port_clk;
		char clk_name[15];

		mscan_clk = clk_get(NULL, "mscan_clk");
		if (!mscan_clk) {
			dev_err(&pdev->dev, "can't get mscan clock!");
			ret = -EINVAL;
			goto fail_map;
		}

		sprintf(clk_name, "mscan%d_clk", pdata->port);
		port_clk = clk_get(NULL, clk_name);

		/* update clock rate for mpc5121e rev2 chip */
		if (port_clk)
			pdata->clock_frq = clk_get_rate(port_clk);

		/* enable clock for mscan module */
		clk_enable(mscan_clk);
	}

	can->can_sys_clock = pdata->clock_frq;

	platform_set_drvdata(pdev, dev);

	ret = register_mscandev(dev, pdata->clock_src);
	if (ret >= 0) {
		dev_info(&pdev->dev, "probe port 0x%lX done, clk rate:%d\n",
			 dev->base_addr, pdata->clock_frq);
		return ret;
	}

	iounmap((unsigned long *)dev->base_addr);
      fail_map:
	release_mem_region(mem->start, mem_size);
      req_error:
	free_candev(dev);
	dev_err(&pdev->dev, "probe failed\n");
	return ret;
}

static int __devexit mpc52xx_can_remove(struct platform_device *pdev)
{
	struct net_device *dev = platform_get_drvdata(pdev);
	struct resource *mem;

	platform_set_drvdata(pdev, NULL);
	unregister_mscandev(dev);

	iounmap((volatile void __iomem *)dev->base_addr);
	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	release_mem_region(mem->start, mem->end - mem->start + 1);
	free_candev(dev);
	return 0;
}

#ifdef CONFIG_PM
static struct mscan_regs saved_regs;
static int mpc52xx_can_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct net_device *dev = platform_get_drvdata(pdev);
	struct mscan_regs *regs = (struct mscan_regs *)dev->base_addr;

	_memcpy_fromio(&saved_regs, regs, sizeof(*regs));

	regs->canctl1 |= MSCAN_CANE;
	regs->canctl1 &= ~MSCAN_LISTEN;
	regs->canctl0 |= MSCAN_SLPRQ;
	regs->canctl0 |= MSCAN_INITRQ;
	mdelay(20);
	regs->canctl0 &= ~MSCAN_INITRQ;
	regs->canctl0 |= MSCAN_WUPE;
	mdelay(20);
	regs->canrier |= 0xff;

	return 0;
}

static int mpc52xx_can_resume(struct platform_device *pdev)
{
	struct net_device *dev = platform_get_drvdata(pdev);
	struct mscan_regs *regs = (struct mscan_regs *)dev->base_addr;

	regs->canctl0 |= MSCAN_INITRQ;
	while ((regs->canctl1 & MSCAN_INITAK) == 0)
		udelay(10);

	regs->canctl1 = saved_regs.canctl1;
	regs->canbtr0 = saved_regs.canbtr0;
	regs->canbtr1 = saved_regs.canbtr1;
	regs->canidac = saved_regs.canidac;

	/* restore masks, buffers etc. */
	_memcpy_toio(&regs->canidar1_0, (void *)&saved_regs.canidar1_0,
		     sizeof(*regs) - offsetof(struct mscan_regs, canidar1_0));

	regs->canctl0 &= ~MSCAN_INITRQ;
	regs->cantbsel = saved_regs.cantbsel;
	regs->canrier = saved_regs.canrier;
	regs->cantier = saved_regs.cantier;
	regs->canctl0 = saved_regs.canctl0;

	regs->canrflg &= 0xc3;

	return 0;
}
#endif

static struct platform_driver mpc52xx_can_driver = {
	.driver = {
		   .name = "fsl-mscan",
		   },
	.probe = mpc52xx_can_probe,
	.remove = __devexit_p(mpc52xx_can_remove),
#ifdef CONFIG_PM
	.suspend = mpc52xx_can_suspend,
	.resume = mpc52xx_can_resume,
#endif
};

#ifdef CONFIG_PPC_MERGE
unsigned int fsl_find_ipb_freq(struct device_node *node)
{
	struct device_node *np;
	const unsigned int *p_ipb_freq = NULL;

	of_node_get(node);
	while (node) {
		p_ipb_freq = of_get_property(node, "bus-frequency", NULL);
		if (p_ipb_freq)
			break;

		np = of_get_parent(node);
		of_node_put(node);
		node = np;
	}
	if (node)
		of_node_put(node);

	return p_ipb_freq ? *p_ipb_freq : 0;
}

static int __init mpc52xx_of_to_pdev(void)
{
	struct device_node *np = NULL;
	unsigned int i;
	int ret, type = -1, index = 0;
	int *port;
	char *mscan_comp_name[] = {"fsl,mpc5200-mscan",
				   "fsl,mpc5121rev2-mscan",
				   "fsl,mpc5121-mscan"};
	int  cpu_type[] = {MPC52xx_MSCAN, MPC512x_MSCAN, MPC512x_MSCAN};

	for (i = 0; i < 3; i++) {
		np = of_find_compatible_node(np, NULL, mscan_comp_name[i]);
		if (np) {
			type = cpu_type[i];
			index = i;
			of_node_put(np);
			np = NULL;
			break;
		}
	}

	if (type != cpu_type[0] && type != cpu_type[1]) {
		printk(KERN_ERR "%s: can't find any CAN devices\n", __func__);
		return -1;
	}

	for (i = 0;
	     (np = of_find_compatible_node(np, NULL,
					   mscan_comp_name[index]));
	     i++) {
		struct resource r[2] = { };
		struct mscan_platform_data pdata;

		if (i >= PDEV_MAX) {
			printk(KERN_WARNING "%s: increase PDEV_MAX for more "
			       "than %i devices\n", __func__, PDEV_MAX);
			break;
		}

		ret = of_address_to_resource(np, 0, &r[0]);
		if (ret)
			goto err;

		of_irq_to_resource(np, 0, &r[1]);

		pdev[i] =
		    platform_device_register_simple("fsl-mscan", i, r, 2);
		if (IS_ERR(pdev[i])) {
			ret = PTR_ERR(pdev[i]);
			goto err;
		}

		pdata.clock_src = MSCAN_CLKSRC_BUS;
		pdata.cpu_type = type;
		if (pdata.cpu_type == MPC512x_MSCAN) pdata.clock_src = 0;
		pdata.clock_frq = fsl_find_ipb_freq(np);

		if (pdata.cpu_type == MPC512x_MSCAN) {
			port = (int *)of_get_property(np, "cell-index", NULL);
			if (!port) {
				printk(KERN_ERR "Err: can't find can port!\n");
				goto err;
			}
			pdata.port = *port;
		}

		ret = platform_device_add_data(pdev[i], &pdata, sizeof(pdata));
		if (ret)
			goto err;
	}
	return 0;
      err:
	return ret;
}
#else
#define mscan_of_to_pdev()
#endif

int __init mpc52xx_can_init(void)
{
	mpc52xx_of_to_pdev();
	printk(KERN_INFO "%s initializing\n", mpc52xx_can_driver.driver.name);
	return platform_driver_register(&mpc52xx_can_driver);
}

void __exit mpc52xx_can_exit(void)
{
	int i;
	platform_driver_unregister(&mpc52xx_can_driver);
	for (i = 0; i < PDEV_MAX; i++)
		platform_device_unregister(pdev[i]);
	printk(KERN_INFO "%s unloaded\n", mpc52xx_can_driver.driver.name);
}

module_init(mpc52xx_can_init);
module_exit(mpc52xx_can_exit);

MODULE_AUTHOR("Andrey Volkov <avolkov@varma-el.com>");
MODULE_DESCRIPTION("Freescale MPC5200/MPC512x CAN driver");
MODULE_LICENSE("GPL v2");
