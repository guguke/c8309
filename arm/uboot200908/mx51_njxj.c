/*
 * (C) Copyright 2009 Freescale Semiconductor, Inc.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/iomux-mx51.h>
#include <asm/errno.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/crm_regs.h>
#include <asm/arch/clock.h>
#include <asm/imx-common/mx5_video.h>
#include <i2c.h>
#include <mmc.h>
#include <fsl_esdhc.h>
#include <power/pmic.h>
#include <fsl_pmic.h>
#include <mc13892.h>
#include <usb/ehci-fsl.h>

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_FSL_ESDHC
struct fsl_esdhc_cfg esdhc_cfg[2] = {
        {MMC_SDHC1_BASE_ADDR},
        {MMC_SDHC2_BASE_ADDR},
};
#endif

int board_access_gpio(int pin, char rwc, int * value)
{
	int group;
	u32 base;
	u32 r;

//	printf("geek ----- 0x%x\n", readl(0x73fa8270));

	group=pin >> 16;
	pin &= 0xffff;
	switch(group){
	case 1:
		base=GPIO1_BASE_ADDR;
		break;
	case 2:
		base=GPIO2_BASE_ADDR;
		break;
	case 3:
		base=GPIO3_BASE_ADDR;
		break;
	case 4:
		base=GPIO4_BASE_ADDR;
		break;
	default:
		printf("0x%x invalid gpio spec.(should be group_num <<16 + pin_num)\n", (group<<16) | pin);
		return -1;
	}
	if(pin > 31){
		printf("0x%x invalid gpio spec.(should be group_num <<16 + pin_num)\n", (group<<16) | pin);
		return -1;
	}
	switch(rwc){
	case 'r':
	case 0:
		r=(readl(base) & (1<<pin)) ? 1 : 0;
		if(value)
			*value=r;
		else
			printf("GPIO%d[%d]=%d\n", group, pin, r);
		break;
	case 'w':
	case 1:
		r=readl(base);
		if(value)
			r |= (1<<pin);
		else
			r &= ~(1<<pin);
		writel(r, base);
		break;
	case 'c':
	case 2:
		r=readl(base+4);
		printf("GPIO%d[%d] is %s\n", group, pin, r & (1<<pin) ? "output" : "input");
		break;
	case 'd':	/* set input */
	case 3:
	case 'i':
		r=readl(base+4);
		r &= ~(1<<pin);
		writel(r, base+4);
		break;
	case 'D':	/* set output */
	case 'o':
	case 4:
		r=readl(base+4);
		r |= (1<<pin);
		writel(r, base+4);
		break;
	default:
		printf("Invalid rwc encoding, should be one char in \"rwcdD\"");
		return -1;
	}
	return 0;
}


int dram_init(void)
{
#if 0
        writel(0x4, 0x73FA882c);   /* 0x082c (IOMUXC_SW_PAD_CTL_GRP_DRAM_B4) */
        writel(0x4, 0x73FA88a4);   /* 0x08a4 (IOMUXC_SW_PAD_CTL_GRP_DRAM_B0) */
        writel(0x4, 0x73FA88ac);   /* 0x08ac (IOMUXC_SW_PAD_CTL_GRP_DRAM_B1) */
        writel(0x4, 0x73FA88b8);   /* 0x08b8 (IOMUXC_SW_PAD_CTL_GRP_DRAM_B2) */

        /* 0: Slow Slew Rate */
        /* 1: Fast Slew Rate */
        /* writel(0x0, 0x73FA8878);   /\* 0x0878 (IOMUXC_SW_PAD_CTL_GRP_DRAM_SR_B0); *\/ */
        /* writel(0x0, 0x73FA8880);   /\* 0x0880 (IOMUXC_SW_PAD_CTL_GRP_DRAM_SR_B1); *\/ */
        /* writel(0x0, 0x73FA888c);   /\* 0x088c (IOMUXC_SW_PAD_CTL_GRP_DRAM_SR_B2); *\/ */
        /* writel(0x0, 0x73FA889c);   /\* 0x089c (IOMUXC_SW_PAD_CTL_GRP_DRAM_SR_B4); *\/ */

        /*
         * Setting DDR for micron
         * 13 Rows, 10 Cols, 32 bit, SREF=4 Micron Model
         * CAS=3 BL=4
         */
        writel(0xc2a20000, 0x83FD9000); /* ESDCTL_ESDCTL0 */
        writel(0x42a20000, 0x83FD9008); /* ESDCTL_ESDCTL1 */
        writel(0x000ad0d0, 0x83FD9010); /* ESDCTL_ESDMISC */
        writel(0x333584ab, 0x83FD9004); /* ESDCTL_ESDCFG0 */
        writel(0x333584ab, 0x83FD900C); /* ESDCTL_ESDCFG1 */
#endif
        /* dram_init must store complete ramsize in gd->ram_size */
        gd->ram_size = get_ram_size((void *)CONFIG_SYS_SDRAM_BASE,
                                PHYS_SDRAM_1_SIZE);
        return 0;
}

/* void dram_bank_mmu_setup(int bank) */
/* { */
/*     return; */
/* } */

u32 get_board_rev(void)
{
        u32 rev = get_cpu_rev();
        if (!gpio_get_value(IMX_GPIO_NR(1, 22)))
                rev |= BOARD_REV_2_0 << BOARD_VER_OFFSET;
        return rev;
}

static void setup_wdog_13892(void)
{
        /* GPIO1[4] is connected to the 13892 WDI pin, set it to high */
        static const iomux_v3_cfg_t gpio1_4_pads[] = {
                MX51_PAD_GPIO1_4__GPIO1_4,
        };
        imx_iomux_v3_setup_multiple_pads(gpio1_4_pads, ARRAY_SIZE(gpio1_4_pads));

        /* // MX51_PAD_GPIO1_4__GPIO1_4=wdog_b: */
	/* writel(0x000, 0x73fa83d8);  /\* 0x03d8 (IOMUXC_SW_MUX_CTL_PAD_GPIO1_4) *\/ */
	/* writel(0x120, 0x73fa8804);  /\* 0x0804 (IOMUXC_SW_PAD_CTL_PAD_GPIO1_4) *\/ */

        int tmpval = 0;
        board_access_gpio(0x00010004, 'o', &tmpval); /* set output */
        board_access_gpio(0x00010004, 'w', &tmpval); /* set PMIC_WDI to high */

        return;
}

#define UART_PAD_CTRL   (PAD_CTL_HYS | PAD_CTL_PUS_100K_DOWN | PAD_CTL_DSE_HIGH)

static void setup_iomux_uart(void)
{
        static const iomux_v3_cfg_t uart_pads[] = {
                MX51_PAD_UART1_RXD__UART1_RXD,
                MX51_PAD_UART1_TXD__UART1_TXD,
                MX51_PAD_UART2_RXD__UART2_RXD,
                MX51_PAD_UART2_TXD__UART2_TXD,

                MX51_PAD_UART3_TXD__UART3_TXD,
                MX51_PAD_UART3_RXD__UART3_RXD,

                NEW_PAD_CTRL(MX51_PAD_UART1_RTS__UART1_RTS, UART_PAD_CTRL),
                NEW_PAD_CTRL(MX51_PAD_UART1_CTS__UART1_CTS, UART_PAD_CTRL),
        };

        imx_iomux_v3_setup_multiple_pads(uart_pads, ARRAY_SIZE(uart_pads));

        if(0){ /* init UART2 */
#if 1
                writel(0,     0x73fa8238);
                writel(0x1c4, 0x73fa8628);
                writel(2,     0x73fa89ec);
                writel(0,     0x73fa823c);
                writel(0x1c4, 0x73fa862c);
#else
                const unsigned int PAD_CTL_DRV_HIGH     = 0x2 << 1;
                const unsigned int PAD_CTL_PUE_PULL     = 0x1 << 6;
                const unsigned int PAD_CTL_PKE_ENABLE   = 0x1 << 7;
                const unsigned int PAD_CTL_HYS_ENABLE   = 0x1 << 8;

                const unsigned int IOMUX_CONFIG_ALT0    = 0;
#define IOMUXC_SW_PAD_CTL_PAD_UART2_RXD      (0x73fa8000 + 0x628)
#define IOMUXC_SW_PAD_CTL_PAD_UART2_TXD      (0x73fa8000 + 0x62c)

#define IOMUXC_SW_MUX_CTL_PAD_UART2_RXD      (0x73fa8000 + 0x238)
#define IOMUXC_SW_MUX_CTL_PAD_UART2_TXD      (0x73fa8000 + 0x23c)

#define IOMUXC_UART2_IPP_UART_RTS_B_SELECT_INPUT (0x73fa8000 + 0x9ec)

                unsigned int pad =                              \
                        PAD_CTL_HYS_ENABLE | /* 0x100 */
                        PAD_CTL_PKE_ENABLE | /* 0x80 */
                        PAD_CTL_PUE_PULL |   /* 0x40 */
                        PAD_CTL_DRV_HIGH;    /* 0x04 */
                /* total = 0x1c4 */

                writel(IOMUX_CONFIG_ALT0, IOMUXC_SW_MUX_CTL_PAD_UART2_RXD);
                writel(pad | PAD_CTL_SRE_FAST, IOMUXC_SW_PAD_CTL_PAD_UART2_RXD);

                writel(2, IOMUXC_UART2_IPP_UART_RTS_B_SELECT_INPUT);

                /* 0x73FA80000 + (MUX_IN_UART2_IPP_UART_RXD_MUX_SELECT_INPUT<<2) + 0x8c4/0x928; */
                /* (0x4a<<2)+0x928 =  */
                /*         01 0010 1000 */
                /*         0x128+0x8c4 = 0x9ec */
                writel(IOMUX_CONFIG_ALT0, IOMUXC_SW_MUX_CTL_PAD_UART2_TXD);
                writel(pad | PAD_CTL_SRE_FAST, IOMUXC_SW_PAD_CTL_PAD_UART2_TXD);
#endif
        }
        if(0){ /* test UART2 */
#define UART_PHYS (0x73F00000+0x000C0000)
#define URXD  0x0  /* Receiver Register */
#define UTXD  0x40 /* Transmitter Register */
#define UCR1  0x80 /* Control Register 1 */
#define UCR2  0x84 /* Control Register 2 */
#define UCR3  0x88 /* Control Register 3 */
#define UCR4  0x8c /* Control Register 4 */
#define UFCR  0x90 /* FIFO Control Register */
#define USR1  0x94 /* Status Register 1 */
#define USR2  0x98 /* Status Register 2 */
#define UESC  0x9c /* Escape Character Register */
#define UTIM  0xa0 /* Escape Timer Register */
#define UBIR  0xa4 /* BRM Incremental Register */
#define UBMR  0xa8 /* BRM Modulator Register */
#define UBRC  0xac /* Baud Rate Count Register */
#define UTS   0xb4 /* UART Test Register (mx31) */

#define  UCR1_UARTEN     (1<<0)	 /* UART enabled */
#define  UCR2_ESCI	 (1<<15) /* Escape seq interrupt enable */
#define  UCR2_IRTS	 (1<<14) /* Ignore RTS pin */
#define  UCR2_CTSC	 (1<<13) /* CTS pin control */
#define  UCR2_CTS        (1<<12) /* Clear to send */
#define  UCR2_ESCEN      (1<<11) /* Escape enable */
#define  UCR2_PREN       (1<<8)  /* Parity enable */
#define  UCR2_PROE       (1<<7)  /* Parity odd/even */
#define  UCR2_STPB       (1<<6)	 /* Stop */
#define  UCR2_WS         (1<<5)	 /* Word size */
#define  UCR2_RTSEN      (1<<4)	 /* Request to send interrupt enable */
#define  UCR2_TXEN       (1<<2)	 /* Transmitter enabled */
#define  UCR2_RXEN       (1<<1)	 /* Receiver enabled */
#define  UCR2_SRST	 (1<<0)	 /* SW reset */
#define  UTS_TXEMPTY	 (1<<6)	 /* TxFIFO empty */

                __REG(UART_PHYS + UCR1) = 0x0;
                __REG(UART_PHYS + UCR2) = 0x0;

                while (!(__REG(UART_PHYS + UCR2) & UCR2_SRST));

                __REG(UART_PHYS + UCR3) = 0x0704;
                __REG(UART_PHYS + UCR4) = 0x8000;
                __REG(UART_PHYS + UESC) = 0x002b;
                __REG(UART_PHYS + UTIM) = 0x0;

                __REG(UART_PHYS + UTS) = 0x0;

                /* serial_setbrg(); */
                __REG(UART_PHYS + UFCR) = 4 << 7; /* divide input clock by 2 */
                __REG(UART_PHYS + UBIR) = 0xf;
                __REG(UART_PHYS + UBMR) = 66500000 / (2 * 115200);

                __REG(UART_PHYS + UCR2) = UCR2_WS | UCR2_IRTS | UCR2_RXEN | UCR2_TXEN | UCR2_SRST;

                __REG(UART_PHYS + UCR1) = UCR1_UARTEN;

                if(1){
                        int   i = 0;
                        char  str[128];
                        char* s;
                        for (i=0; ; ++i){
                                sprintf(str, "counter = %d\n", i);
                                s = str;
                                while (*s){
                                        __REG(UART_PHYS + UTXD) = *s;

                                        /* wait for transmitter to be ready */
                                        while (!(__REG(UART_PHYS + UTS) & UTS_TXEMPTY)){
                                        };

                                        if (*s == '\n'){
                                                __REG(UART_PHYS + UTXD) = '\r';
                                                while (!(__REG(UART_PHYS + UTS) & UTS_TXEMPTY)){
                                                };
                                        }

                                        s++;
                                }
                        }
                }
        }
}

void reset_phy(void)
{
	board_access_gpio(0x2001b,'o',0);
	board_access_gpio(0x2001b,'w',(int*)0);
	udelay(10*1000);
	board_access_gpio(0x2001b,'w',(int*)1);
}

static void setup_iomux_fec(void)
{
#if 0
        static const iomux_v3_cfg_t fec_pads[] = {
                NEW_PAD_CTRL(MX51_PAD_EIM_EB2__FEC_MDIO, PAD_CTL_HYS |
                                PAD_CTL_PUS_22K_UP | PAD_CTL_ODE |
                                PAD_CTL_DSE_HIGH | PAD_CTL_SRE_FAST),
                MX51_PAD_NANDF_CS3__FEC_MDC,
                NEW_PAD_CTRL(MX51_PAD_EIM_CS3__FEC_RDATA3, MX51_PAD_CTRL_2),
                NEW_PAD_CTRL(MX51_PAD_EIM_CS2__FEC_RDATA2, MX51_PAD_CTRL_2),
                NEW_PAD_CTRL(MX51_PAD_EIM_EB3__FEC_RDATA1, MX51_PAD_CTRL_2),
                MX51_PAD_NANDF_D9__FEC_RDATA0,
                MX51_PAD_NANDF_CS6__FEC_TDATA3,
                MX51_PAD_NANDF_CS5__FEC_TDATA2,
                MX51_PAD_NANDF_CS4__FEC_TDATA1,
                MX51_PAD_NANDF_D8__FEC_TDATA0,
                MX51_PAD_NANDF_CS7__FEC_TX_EN,
                MX51_PAD_NANDF_CS2__FEC_TX_ER,
                MX51_PAD_NANDF_RDY_INT__FEC_TX_CLK,
                NEW_PAD_CTRL(MX51_PAD_NANDF_RB2__FEC_COL, MX51_PAD_CTRL_4),
                NEW_PAD_CTRL(MX51_PAD_NANDF_RB3__FEC_RX_CLK, MX51_PAD_CTRL_4),
                MX51_PAD_EIM_CS5__FEC_CRS,
                MX51_PAD_EIM_CS4__FEC_RX_ER,
                NEW_PAD_CTRL(MX51_PAD_NANDF_D11__FEC_RX_DV, MX51_PAD_CTRL_4),
        };

        imx_iomux_v3_setup_multiple_pads(fec_pads, ARRAY_SIZE(fec_pads));
#else
        // MX51_PAD_DI2_PIN3__FEC_MDIO:
	writel(0x2, 0x73fa8348);
	writel(0x1, 0x73fa8954);
	writel(0x0, 0x73fa8750);

        // MX51_PAD_DI2_PIN2__FEC_MDC:
	writel(0x2, 0x73fa8344);
	writel(0x2004, 0x73fa874c);

        // MX51_PAD_DISP2_DAT0__FEC_RDATA3:
	writel(0x2, 0x73fa8354);
	writel(0x1, 0x73fa8964);
	writel(0x0, 0x73fa875c);

        // MX51_PAD_DI_GP4__FEC_RDATA2:
	writel(0x2, 0x73fa8350);
	writel(0x1, 0x73fa8960);
	writel(0x0, 0x73fa8758);

        // MX51_PAD_DI2_DISP_CLK__FEC_RDATA1:
	writel(0x2, 0x73fa834c);
	writel(0x1, 0x73fa895c);
	writel(0x0, 0x73fa8754);

        // MX51_PAD_DISP2_DAT14__FEC_RDATA0:
	writel(0x2, 0x73fa838c);
	writel(0x1, 0x73fa8958);
	writel(0x2180, 0x73fa8794);

        // MX51_PAD_DISP2_DAT12__FEC_RX_DV:
	writel(0x2, 0x73fa8384);
	writel(0x1, 0x73fa896c);
	writel(0x0, 0x73fa878c);

        // MX51_PAD_DISP2_DAT11__FEC_RX_CLK:
	writel(0x2, 0x73fa8380);
	writel(0x1, 0x73fa8968);
	writel(0x0, 0x73fa8788);

        // MX51_PAD_DISP2_DAT1__FEC_RX_ER:
	writel(0x2, 0x73fa8358);
	writel(0x1, 0x73fa8970);
	writel(0x0, 0x73fa8760);

        // MX51_PAD_DISP2_DAT13__FEC_TX_CLK:
	writel(0x2, 0x73fa8388);
	writel(0x1, 0x73fa8974);
	writel(0x2180, 0x73fa8790);

        // MX51_PAD_DISP2_DAT9__FEC_TX_EN:
	writel(0x2, 0x73fa8378);
	writel(0x2004, 0x73fa8780);

        // MX51_PAD_DISP2_DAT15__FEC_TDATA0:
	writel(0x2, 0x73fa8390);
	writel(0x2004, 0x73fa8798);

        // MX51_PAD_DISP2_DAT6__FEC_TDATA1:
	writel(0x2, 0x73fa836c);
	writel(0x2004, 0x73fa8774);

        // MX51_PAD_DISP2_DAT7__FEC_TDATA2:
	writel(0x2, 0x73fa8370);
	writel(0x2004, 0x73fa8778);

        // MX51_PAD_DISP2_DAT8__FEC_TDATA3:
	writel(0x2, 0x73fa8374);
	writel(0x2004, 0x73fa877c);

        // MX51_PAD_DISP2_DAT10__FEC_COL:
	writel(0x2, 0x73fa837c);
	writel(0x1, 0x73fa894c);
	writel(0x0, 0x73fa8784);

        // MX51_PAD_DI2_PIN4__FEC_CRS:
	writel(0x2, 0x73fa8340);
	writel(0x1, 0x73fa8950);
	writel(0x0, 0x73fa8748);

        // MX51_PAD_DI_GP3__FEC_TX_ER:
	writel(0x2, 0x73fa833c);
	writel(0x2004, 0x73fa8744);

        // MX51_PAD_EIM_CS2__GPIO2_27:
	writel(0x1, 0x73fa80e8);
	writel(0x85, 0x73fa847c);

#endif
}

#ifdef CONFIG_MXC_SPI
static void setup_iomux_spi(void)
{
        static const iomux_v3_cfg_t spi_pads[] = {
                NEW_PAD_CTRL(MX51_PAD_CSPI1_MOSI__ECSPI1_MOSI, PAD_CTL_HYS | PAD_CTL_DSE_HIGH | PAD_CTL_SRE_FAST),
                NEW_PAD_CTRL(MX51_PAD_CSPI1_MISO__ECSPI1_MISO, PAD_CTL_HYS | PAD_CTL_DSE_HIGH | PAD_CTL_SRE_FAST),
                NEW_PAD_CTRL(MX51_PAD_CSPI1_SS1__ECSPI1_SS1,   MX51_GPIO_PAD_CTRL),
                MX51_PAD_CSPI1_SS0__ECSPI1_SS0,                NEW_PAD_CTRL(MX51_PAD_CSPI1_RDY__ECSPI1_RDY, MX51_PAD_CTRL_2),
                NEW_PAD_CTRL(MX51_PAD_CSPI1_SCLK__ECSPI1_SCLK, PAD_CTL_HYS | PAD_CTL_DSE_HIGH | PAD_CTL_SRE_FAST),
        };

        imx_iomux_v3_setup_multiple_pads(spi_pads, ARRAY_SIZE(spi_pads));
}
#endif

#ifdef CONFIG_USB_EHCI_MX5
#define MX51EVK_USBH1_HUB_RST   IMX_GPIO_NR(1, 7)
#define MX51EVK_USBH1_STP       IMX_GPIO_NR(1, 27)
#define MX51EVK_USB_CLK_EN_B    IMX_GPIO_NR(2, 2)
#define MX51EVK_USB_PHY_RESET   IMX_GPIO_NR(2, 5)

static void setup_usb_h1(void)
{
        static const iomux_v3_cfg_t usb_h1_pads[] = {
                MX51_PAD_USBH1_CLK__USBH1_CLK,
                MX51_PAD_USBH1_DIR__USBH1_DIR,
                MX51_PAD_USBH1_STP__USBH1_STP,
                MX51_PAD_USBH1_NXT__USBH1_NXT,
                MX51_PAD_USBH1_DATA0__USBH1_DATA0,
                MX51_PAD_USBH1_DATA1__USBH1_DATA1,
                MX51_PAD_USBH1_DATA2__USBH1_DATA2,
                MX51_PAD_USBH1_DATA3__USBH1_DATA3,
                MX51_PAD_USBH1_DATA4__USBH1_DATA4,
                MX51_PAD_USBH1_DATA5__USBH1_DATA5,
                MX51_PAD_USBH1_DATA6__USBH1_DATA6,
                MX51_PAD_USBH1_DATA7__USBH1_DATA7,

                NEW_PAD_CTRL(MX51_PAD_GPIO1_7__GPIO1_7, 0), /* H1 hub reset */
                MX51_PAD_EIM_D17__GPIO2_1,
                MX51_PAD_EIM_D21__GPIO2_5, /* PHY reset */
        };

        imx_iomux_v3_setup_multiple_pads(usb_h1_pads, ARRAY_SIZE(usb_h1_pads));
}

void dram_init_banksize(void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size  = PHYS_SDRAM_1_SIZE;

	/* gd->bd->bi_dram[1].start = PHYS_SDRAM_2; */
	/* gd->bd->bi_dram[1].size  = PHYS_SDRAM_2_SIZE; */
}


int board_ehci_hcd_init(int port)
{
        /* Set USBH1_STP to GPIO and toggle it */
        imx_iomux_v3_setup_pad(NEW_PAD_CTRL(MX51_PAD_USBH1_STP__GPIO1_27,
                                                MX51_USBH_PAD_CTRL));

        gpio_direction_output(MX51EVK_USBH1_STP, 0);
        gpio_direction_output(MX51EVK_USB_PHY_RESET, 0);
        mdelay(10);
        gpio_set_value(MX51EVK_USBH1_STP, 1);

        /* Set back USBH1_STP to be function */
        imx_iomux_v3_setup_pad(MX51_PAD_USBH1_STP__USBH1_STP);

        /* De-assert USB PHY RESETB */
        gpio_set_value(MX51EVK_USB_PHY_RESET, 1);

        /* Drive USB_CLK_EN_B line low */
        gpio_direction_output(MX51EVK_USB_CLK_EN_B, 0);

        /* Reset USB hub */
        gpio_direction_output(MX51EVK_USBH1_HUB_RST, 0);
        mdelay(2);
        gpio_set_value(MX51EVK_USBH1_HUB_RST, 1);
        return 0;
}
#endif

static void power_init(void)
{
        unsigned int val;
        struct mxc_ccm_reg *mxc_ccm = (struct mxc_ccm_reg *)MXC_CCM_BASE;
        struct pmic *p;
        int ret;

        ret = pmic_init(I2C_PMIC);
        if (ret)
                return;

        p = pmic_get("FSL_PMIC");
        if (!p)
                return;

        /* Write needed to Power Gate 2 register */
        pmic_reg_read(p, REG_POWER_MISC, &val);
        val &= ~PWGT2SPIEN;
        pmic_reg_write(p, REG_POWER_MISC, val);

        /* Externally powered */
        pmic_reg_read(p, REG_CHARGE, &val);
        val |= ICHRG0 | ICHRG1 | ICHRG2 | ICHRG3 | CHGAUTOB;
        pmic_reg_write(p, REG_CHARGE, val);

        /* power up the system first */
        pmic_reg_write(p, REG_POWER_MISC, PWUP);

        /* Set core voltage to 1.1V */
        pmic_reg_read(p, REG_SW_0, &val);
        val = (val & ~SWx_VOLT_MASK) | SWx_1_100V;
        pmic_reg_write(p, REG_SW_0, val);

        /* Setup VCC (SW2) to 1.075v */
        pmic_reg_read(p, REG_SW_1, &val);
        val = (val & ~SWx_VOLT_MASK) | SWx_1_225V;
        pmic_reg_write(p, REG_SW_1, val);

        /* Setup 1V2_DIG1 (SW3) to 1.05v */
        pmic_reg_read(p, REG_SW_2, &val);
        val = (val & ~SWx_VOLT_MASK) | SWx_1_050V;
        pmic_reg_write(p, REG_SW_2, val);
        udelay(50);

        /* Raise the core frequency to 800MHz */
        writel(0x0, &mxc_ccm->cacrr);

        /* Set switchers in Auto in NORMAL mode & STANDBY mode */
        /* Setup the switcher mode for SW1 & SW2*/
        pmic_reg_read(p, REG_SW_4, &val);
        val = (val & ~((SWMODE_MASK << SWMODE1_SHIFT) |
                (SWMODE_MASK << SWMODE2_SHIFT)));
        val |= (SWMODE_AUTO_AUTO << SWMODE1_SHIFT) |
                (SWMODE_AUTO_AUTO << SWMODE2_SHIFT);
        pmic_reg_write(p, REG_SW_4, val);

        /* Setup the switcher mode for SW3 & SW4 */
        pmic_reg_read(p, REG_SW_5, &val);
        val = (val & ~((SWMODE_MASK << SWMODE3_SHIFT) |
                (SWMODE_MASK << SWMODE4_SHIFT)));
        val |= (SWMODE_AUTO_AUTO << SWMODE3_SHIFT) |
                (SWMODE_AUTO_AUTO << SWMODE4_SHIFT);
        pmic_reg_write(p, REG_SW_5, val);

        /* Set VDIG to 1.65V, VGEN3 to 1.8V, VCAM to 2.6V */
        pmic_reg_read(p, REG_SETTING_0, &val);
        val &= ~(VCAM_MASK | VGEN3_MASK | VDIG_MASK);
        val |= VDIG_1_65 | VGEN3_1_8 | VCAM_2_6;
        pmic_reg_write(p, REG_SETTING_0, val);

        /* Set VVIDEO to 2.775V, VAUDIO to 3V, VSD to 3.15V */
        pmic_reg_read(p, REG_SETTING_1, &val);
        val &= ~(VVIDEO_MASK | VSD_MASK | VAUDIO_MASK);
        val |= VSD_3_15 | VAUDIO_3_0 | VVIDEO_2_775;
        pmic_reg_write(p, REG_SETTING_1, val);

        /* Configure VGEN3 and VCAM regulators to use external PNP */
        val = VGEN3CONFIG | VCAMCONFIG;
        pmic_reg_write(p, REG_MODE_1, val);
        udelay(200);

        /* Enable VGEN3, VCAM, VAUDIO, VVIDEO, VSD regulators */
        val = VGEN3EN | VGEN3CONFIG | VCAMEN | VCAMCONFIG |
                VVIDEOEN | VAUDIOEN  | VSDEN;
        pmic_reg_write(p, REG_MODE_1, val);

        if(1){/* testing LED */
                unsigned int led_setting;
                unsigned int driver_current         = 0x04; /* [11:9] */
                unsigned int drivers_duty_cycle     = 0x20; /* [8:3]  */
                unsigned int drivers_period_control = 0x03; /* [1:0]  */
                unsigned int drivers_ramp           = 0;    /* [3]    */

                led_setting = (driver_current<<9) | (drivers_duty_cycle<<3) | (drivers_ramp<<2) | (drivers_period_control);
                pmic_reg_write(p, REG_LED_CTL2, (val<<11)|val);
                pmic_reg_write(p, REG_LED_CTL3, val);
        }
        if(1){/* testing LED D7 */
                int i;
                u32 val;

                /* 0x73fa86bc bit[2:1] set 1 */
                /* IOMUXC_SW_PAD_CTL_PAD_DISPB2_SER_DIN DSE(Drive Strength Field) set to max strength */
                val  = readl(0x73fa86bc);
                val |= 0x06;
                writel(val, 0x73fa86bc);


                /* 0x73fa82bc bit[2] set to 1 */
                /* IOMUXC_SW_MUX_CTL_PAD_DISPB2_SER_DIN MUX_MODE set to */
                /* 0x100, ALT4 mux port: GPIO[5] of instance: gpio3. */
                /* DISPB2_SER_DIN to be gpio3[5] */
                /* Select mux mode: ALT4 mux port: GPIO[5] of instance: gpio3. */
                writel(0x04, 0x73fa82bc);

                /* 0x73fa8988 bit[0] set to 1 */
                /* IOMUXC_GPIO3_IPP_IND_G_IN_5_SELECT_INPUT DAISY */
                /* Selecting BGA contact: DISPB2_SER_DIN for Mode: ALT4. */
                val  = readl(0x73fa8988);
                val |= 0x01;
                writel(val, 0x73fa8988);

                /* 0x73f8c000---0x73f8ffff is GPIO3 IO memory map */

                /* 0x73f8c004 bit[5] set to 1 */
                /*   let GPIO[5] to be output direction */
                /*   GPIO Direction: */
                /*      0 GPIO is configured as input. */
                /*      1 GPIO is configured as output. */
                val  = readl(0x73f8c004);
                val |= 0x20;
                writel(val, 0x73f8c004);

                /* 0x73f8c014 bit[7:0] clear */
                /*  diable the least 8 pin interrupt. Seems too strong! */
                /*  I think only GPIO[5] should be disabled */
                /*   GPIO Interrupt Mask Register */
                /*      0 The interrupt i is disabled. */
                /*      1 The interrupt i is enabled. */
                val  = readl(0x73f8c014);
                val &= ~0x20;
                writel(val, 0x73f8c014);

                /* // MX51_PAD_DISPB2_SER_DIN__GPIO3_5=lcd_led_en: */
                /* writel(0x04, 0x73fa82bc); */
                /* writel(0x01, 0x73fa8988); */
                /* writel(0x85, 0x73fa86bc); */

                board_access_gpio(0x30005, 'o', 0); /* set output */

                for(i=0; i<5; ++i){
                        printf("loop for test LED: %d\n", i);
                        board_access_gpio(0x30005, 'w', 1);
                        udelay(1000*50);
                        board_access_gpio(0x30005, 'w', 0);
                        udelay(1000*50);
                }
        }

        imx_iomux_v3_setup_pad(NEW_PAD_CTRL(MX51_PAD_EIM_A20__GPIO2_14,
                                                NO_PAD_CTRL));
        gpio_direction_output(IMX_GPIO_NR(2, 14), 0);

        udelay(500);

        gpio_set_value(IMX_GPIO_NR(2, 14), 1);
}

#ifdef CONFIG_FSL_ESDHC
int board_mmc_getcd(struct mmc *mmc)
{
        struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
        int ret;

        imx_iomux_v3_setup_pad(NEW_PAD_CTRL(MX51_PAD_GPIO1_0__GPIO1_0,
                                                NO_PAD_CTRL));
        gpio_direction_input(IMX_GPIO_NR(1, 0));
        imx_iomux_v3_setup_pad(NEW_PAD_CTRL(MX51_PAD_GPIO1_6__GPIO1_6,
                                                NO_PAD_CTRL));
        gpio_direction_input(IMX_GPIO_NR(1, 6));

        if (cfg->esdhc_base == MMC_SDHC1_BASE_ADDR)
                ret = !gpio_get_value(IMX_GPIO_NR(1, 0));
        else
                ret = !gpio_get_value(IMX_GPIO_NR(1, 6));

        return ret;
}

int board_mmc_init(bd_t *bis)
{
        static const iomux_v3_cfg_t sd1_pads[] = {
                NEW_PAD_CTRL(MX51_PAD_SD1_CMD__SD1_CMD, PAD_CTL_DSE_MAX |
                        PAD_CTL_HYS | PAD_CTL_PUS_47K_UP | PAD_CTL_SRE_FAST),
                NEW_PAD_CTRL(MX51_PAD_SD1_CLK__SD1_CLK, PAD_CTL_DSE_MAX |
                        PAD_CTL_PUS_47K_UP | PAD_CTL_SRE_FAST),
                NEW_PAD_CTRL(MX51_PAD_SD1_DATA0__SD1_DATA0, PAD_CTL_DSE_MAX |
                        PAD_CTL_HYS | PAD_CTL_PUS_47K_UP | PAD_CTL_SRE_FAST),
                NEW_PAD_CTRL(MX51_PAD_SD1_DATA1__SD1_DATA1, PAD_CTL_DSE_MAX |
                        PAD_CTL_HYS | PAD_CTL_PUS_47K_UP | PAD_CTL_SRE_FAST),
                NEW_PAD_CTRL(MX51_PAD_SD1_DATA2__SD1_DATA2, PAD_CTL_DSE_MAX |
                        PAD_CTL_HYS | PAD_CTL_PUS_47K_UP | PAD_CTL_SRE_FAST),
                NEW_PAD_CTRL(MX51_PAD_SD1_DATA3__SD1_DATA3, PAD_CTL_DSE_MAX |
                        PAD_CTL_HYS | PAD_CTL_PUS_100K_DOWN | PAD_CTL_SRE_FAST),
                NEW_PAD_CTRL(MX51_PAD_GPIO1_0__SD1_CD, PAD_CTL_HYS),
                NEW_PAD_CTRL(MX51_PAD_GPIO1_1__SD1_WP, PAD_CTL_HYS),
        };

        static const iomux_v3_cfg_t sd2_pads[] = {
                NEW_PAD_CTRL(MX51_PAD_SD2_CMD__SD2_CMD,
                                PAD_CTL_DSE_MAX | PAD_CTL_SRE_FAST),
                NEW_PAD_CTRL(MX51_PAD_SD2_CLK__SD2_CLK,
                                PAD_CTL_DSE_MAX | PAD_CTL_SRE_FAST),
                NEW_PAD_CTRL(MX51_PAD_SD2_DATA0__SD2_DATA0,
                                PAD_CTL_DSE_MAX | PAD_CTL_SRE_FAST),
                NEW_PAD_CTRL(MX51_PAD_SD2_DATA1__SD2_DATA1,
                                PAD_CTL_DSE_MAX | PAD_CTL_SRE_FAST),
                NEW_PAD_CTRL(MX51_PAD_SD2_DATA2__SD2_DATA2,
                                PAD_CTL_DSE_MAX | PAD_CTL_SRE_FAST),
                NEW_PAD_CTRL(MX51_PAD_SD2_DATA3__SD2_DATA3,
                                PAD_CTL_DSE_MAX | PAD_CTL_SRE_FAST),
                NEW_PAD_CTRL(MX51_PAD_GPIO1_6__GPIO1_6, PAD_CTL_HYS),
                NEW_PAD_CTRL(MX51_PAD_GPIO1_5__GPIO1_5, PAD_CTL_HYS),
        };

        u32 index;
        s32 status = 0;

        esdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC_CLK);
        esdhc_cfg[1].sdhc_clk = mxc_get_clock(MXC_ESDHC2_CLK);

        for (index = 0; index < CONFIG_SYS_FSL_ESDHC_NUM;
                        index++) {
                switch (index) {
                case 0:
                        imx_iomux_v3_setup_multiple_pads(sd1_pads,
                                                         ARRAY_SIZE(sd1_pads));
                        break;
                case 1:
                        imx_iomux_v3_setup_multiple_pads(sd2_pads,
                                                         ARRAY_SIZE(sd2_pads));
                        break;
                default:
                        printf("Warning: you configured more ESDHC controller"
                                "(%d) as supported by the board(2)\n",
                                CONFIG_SYS_FSL_ESDHC_NUM);
                        return status;
                }
                status |= fsl_esdhc_initialize(bis, &esdhc_cfg[index]);
        }
        return status;
}
#endif

int board_early_init_f(void)
{
        setup_wdog_13892();
        setup_iomux_uart();
        setup_iomux_fec();
#ifdef CONFIG_USB_EHCI_MX5
        setup_usb_h1();
#endif
        setup_iomux_lcd();

        return 0;
}

int board_init(void)
{
        /* address of boot parameters */
        gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x100;

        return 0;
}

#ifdef CONFIG_BOARD_LATE_INIT
int board_late_init(void)
{
#ifdef CONFIG_MXC_SPI
        setup_iomux_spi();
        power_init();
#endif

        reset_phy();

#ifdef CONFIG_HW_WATCHDOG
        if(1){
                /* NOTE:                                                         */
                /*     hw_watchdog_init() seems not work, use the following code */
                /*     which is stolen from linux kerenel to init the watchdog.  */
                /*     hw_watchdog_reset() works fine!                           */
#define IMX2_WDT_WCR_WT		(0xFF << 8)	/* -> Watchdog Timeout Field */
#define IMX2_WDT_WCR_WRE	(1 << 3)	/* -> WDOG Reset Enable */
#define IMX2_WDT_WCR_WDE	(1 << 2)	/* -> Watchdog Enable */
#define WDOG_SEC_TO_COUNT(s)	((s * 2 - 1) << 8)

                u16 val = readw(WDOG1_BASE_ADDR);

                /* Strip the old watchdog Time-Out value */
                val &= ~IMX2_WDT_WCR_WT;
                /* Generate reset if WDOG times out */
                val &= ~IMX2_WDT_WCR_WRE;
                /* Keep Watchdog Disabled */
                val &= ~IMX2_WDT_WCR_WDE;
                /* Set the watchdog's Time-Out value */
                val |= WDOG_SEC_TO_COUNT(CONFIG_WATCHDOG_TIMEOUT_MSECS/1000);

                writew(val, WDOG1_BASE_ADDR);

                /* enable the watchdog */
                val |= IMX2_WDT_WCR_WDE;
                writew(val, WDOG1_BASE_ADDR);

                /* According to i,MX515 Reference Manual, the PDE bit */
                /* should be cleared within 16 second after boot */

                /* Write to the PDE (Power Down Enable) bit */
                writew(0, WDOG1_BASE_ADDR+8);

#undef IMX2_WDT_WCR_WT
#undef IMX2_WDT_WCR_WRE
#undef IMX2_WDT_WCR_WDE
#undef WDOG_SEC_TO_COUNT
        }

#endif
        return 0;
}
#endif

/*
 * Do not overwrite the console
 * Use always serial for U-Boot console
 */
int overwrite_console(void)
{
        return 1;
}

int checkboard(void)
{
        puts("Board: MX51-NanJing.XiaoJin\n");
#if 0
        printf("watch dog_1 CFG = 0x%04x\n", readw(WDOG1_BASE_ADDR));
        printf("watch dog_2 CFG = 0x%04x\n", readw(WDOG2_BASE_ADDR));

        printf("(0x73fa8104) IOMUXC_SW_MUX_CTL_PAD_DRAM_CS1    = 0x%08x\n", readl(0x73fa8104));
        printf("(0x73fa84a4) IOMUXC_SW_PAD_CTL_PAD_DRAM_RAS    = 0x%08x\n", readl(0x73fa84a4));
        printf("(0x73fa84a8) IOMUXC_SW_PAD_CTL_PAD_DRAM_CAS    = 0x%08x\n", readl(0x73fa84a8));
        printf("(0x73fa84ac) IOMUXC_SW_PAD_CTL_PAD_DRAM_SDWE   = 0x%08x\n", readl(0x73fa84ac));
        printf("(0x73fa84b0) IOMUXC_SW_PAD_CTL_PAD_DRAM_SDCKE0 = 0x%08x\n", readl(0x73fa84b0));
        printf("(0x73fa84b4) IOMUXC_SW_PAD_CTL_PAD_DRAM_SDCKE1 = 0x%08x\n", readl(0x73fa84b4));
        printf("(0x73fa84b8) IOMUXC_SW_PAD_CTL_PAD_DRAM_SDCLK  = 0x%08x\n", readl(0x73fa84b8));
        printf("(0x73fa84bc) IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS0  = 0x%08x\n", readl(0x73fa84bc));
        printf("(0x73fa84c0) IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS1  = 0x%08x\n", readl(0x73fa84c0));
        printf("(0x73fa84c4) IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS2  = 0x%08x\n", readl(0x73fa84c4));
        printf("(0x73fa84c8) IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS3  = 0x%08x\n", readl(0x73fa84c8));
        printf("(0x73fa84cc) IOMUXC_SW_PAD_CTL_PAD_DRAM_CS0    = 0x%08x\n", readl(0x73fa84cc));
        printf("(0x73fa84d0) IOMUXC_SW_PAD_CTL_PAD_DRAM_CS1    = 0x%08x\n", readl(0x73fa84d0));
        printf("(0x73fa84d4) IOMUXC_SW_PAD_CTL_PAD_DRAM_DQM0   = 0x%08x\n", readl(0x73fa84d4));
        printf("(0x73fa84d8) IOMUXC_SW_PAD_CTL_PAD_DRAM_DQM1   = 0x%08x\n", readl(0x73fa84d8));
        printf("(0x73fa84dc) IOMUXC_SW_PAD_CTL_PAD_DRAM_DQM2   = 0x%08x\n", readl(0x73fa84dc));
        printf("(0x73fa84e0) IOMUXC_SW_PAD_CTL_PAD_DRAM_DQM3   = 0x%08x\n", readl(0x73fa84e0));
        printf("(0x73fa8508) IOMUXC_SW_PAD_CTL_PAD_EIM_SDBA2   = 0x%08x\n", readl(0x73fa8508));
        printf("(0x73fa850c) IOMUXC_SW_PAD_CTL_PAD_EIM_SDODT1  = 0x%08x\n", readl(0x73fa850c));
        printf("(0x73fa8510) IOMUXC_SW_PAD_CTL_PAD_EIM_SDODT0  = 0x%08x\n", readl(0x73fa8510));
        printf("(0x73fa8820) IOMUXC_SW_PAD_CTL_GRP_DDRPKS      = 0x%08x\n", readl(0x73fa8820));
        printf("(0x73fa882c) IOMUXC_SW_PAD_CTL_GRP_DRAM_B4     = 0x%08x\n", readl(0x73fa882c));
        printf("(0x73fa8830) IOMUXC_SW_PAD_CTL_GRP_INDDR       = 0x%08x\n", readl(0x73fa8830));
        printf("(0x73fa8838) IOMUXC_SW_PAD_CTL_GRP_PKEDDR      = 0x%08x\n", readl(0x73fa8838));
        printf("(0x73fa883c) IOMUXC_SW_PAD_CTL_GRP_DDR_A0      = 0x%08x\n", readl(0x73fa883c));
        printf("(0x73fa8848) IOMUXC_SW_PAD_CTL_GRP_DDR_A1      = 0x%08x\n", readl(0x73fa8848));
        printf("(0x73fa884c) IOMUXC_SW_PAD_CTL_GRP_DDRAPUS     = 0x%08x\n", readl(0x73fa884c));
        printf("(0x73fa885c) IOMUXC_SW_PAD_CTL_GRP_HYSDDR0     = 0x%08x\n", readl(0x73fa885c));
        printf("(0x73fa8864) IOMUXC_SW_PAD_CTL_GRP_HYSDDR1     = 0x%08x\n", readl(0x73fa8864));
        printf("(0x73fa886c) IOMUXC_SW_PAD_CTL_GRP_HYSDDR2     = 0x%08x\n", readl(0x73fa886c));
        printf("(0x73fa8870) IOMUXC_SW_PAD_CTL_GRP_HVDDR       = 0x%08x\n", readl(0x73fa8870));
        printf("(0x73fa8874) IOMUXC_SW_PAD_CTL_GRP_HYSDDR3     = 0x%08x\n", readl(0x73fa8874));
        printf("(0x73fa887c) IOMUXC_SW_PAD_CTL_GRP_DDRAPKS     = 0x%08x\n", readl(0x73fa887c));
        printf("(0x73fa8880) IOMUXC_SW_PAD_CTL_GRP_DRAM_SR_B1  = 0x%08x\n", readl(0x73fa8880));
        printf("(0x73fa8884) IOMUXC_SW_PAD_CTL_GRP_DDRPUS      = 0x%08x\n", readl(0x73fa8884));
        printf("(0x73fa888c) IOMUXC_SW_PAD_CTL_GRP_DRAM_SR_B2  = 0x%08x\n", readl(0x73fa888c));
        printf("(0x73fa8890) IOMUXC_SW_PAD_CTL_GRP_PKEADDR     = 0x%08x\n", readl(0x73fa8890));
        printf("(0x73fa889c) IOMUXC_SW_PAD_CTL_GRP_DRAM_SR_B4  = 0x%08x\n", readl(0x73fa889c));
        printf("(0x73fa88a0) IOMUXC_SW_PAD_CTL_GRP_INMODE1     = 0x%08x\n", readl(0x73fa88a0));
        printf("(0x73fa88a4) IOMUXC_SW_PAD_CTL_GRP_DRAM_B0     = 0x%08x\n", readl(0x73fa88a4));
        printf("(0x73fa88ac) IOMUXC_SW_PAD_CTL_GRP_DRAM_B1     = 0x%08x\n", readl(0x73fa88ac));
        printf("(0x73fa88b0) IOMUXC_SW_PAD_CTL_GRP_DDR_SR_A0   = 0x%08x\n", readl(0x73fa88b0));
        printf("(0x73fa88b8) IOMUXC_SW_PAD_CTL_GRP_DRAM_B2     = 0x%08x\n", readl(0x73fa88b8));
        printf("(0x73fa88bc) IOMUXC_SW_PAD_CTL_GRP_DDR_SR_A1   = 0x%08x\n", readl(0x73fa88bc));

        printf("(0x83FD_9000) ESDCTL_ESDCTL0  = 0x%08x\n", readl(0x83FD9000));
        printf("(0x83FD_9004) ESDCTL_ESDCFG0  = 0x%08x\n", readl(0x83FD9004));
        printf("(0x83FD_9008) ESDCTL_ESDCTL1  = 0x%08x\n", readl(0x83FD9008));
        printf("(0x83FD_900C) ESDCTL_ESDCFG1  = 0x%08x\n", readl(0x83FD900C));
        printf("(0x83FD_9010) ESDCTL_ESDMISC  = 0x%08x\n", readl(0x83FD9010));
        printf("(0x83FD_9014) ESDSCR          = 0x%08x\n", readl(0x83FD9014));
        printf("(0x83FD_9020) ESDCDLY1        = 0x%08x\n", readl(0x83FD9020));
        printf("(0x83FD_9024) ESDCDLY2        = 0x%08x\n", readl(0x83FD9024));
        printf("(0x83FD_9028) ESDCDLY3        = 0x%08x\n", readl(0x83FD9028));
        printf("(0x83FD_902C) ESDCDLY4        = 0x%08x\n", readl(0x83FD902C));
        printf("(0x83FD_9030) ESDCDLY5        = 0x%08x\n", readl(0x83FD9030));
        printf("(0x83FD_9034) ESDGPR          = 0x%08x\n", readl(0x83FD9034));
        printf("(0x83FD_9038) ESDPRCT0        = 0x%08x\n", readl(0x83FD9038));
        printf("(0x83FD_903C) ESDPRCT1        = 0x%08x\n", readl(0x83FD903C));
#endif
        return 0;
}
