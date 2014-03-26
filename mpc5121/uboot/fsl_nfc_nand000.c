/*
 * Copyright (C) 2004-2009 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * Based on drivers/mtd/nand/mpc5121_nand.c
 * which was based on drivers/mtd/nand/mxc_nd.c
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <common.h>
#include <malloc.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>

#include <asm/io.h>
#include <nand.h>

#define MIN(x, y)		((x < y) ? x : y)

static struct fsl_nfc_private {
	struct mtd_info mtd;
	char spare_only;
	char status_req;
	u16 col_addr;
	int writesize;
	int sparesize;
	int width;
	int chipsel;
} *priv;

#define IS_2K_PAGE_NAND		(priv->writesize == 2048)
#define IS_4K_PAGE_NAND		(priv->writesize == 4096)
#define IS_LARGE_PAGE_NAND	(priv->writesize > 512)

#define NFC_REG_BASE ((void *)CONFIG_SYS_NAND_BASE)
/*
 * FSL NFC registers Definition
 */
#define NFC_BUF_ADDR			(NFC_REG_BASE + 0x1E04)
#define NFC_FLASH_ADDR			(NFC_REG_BASE + 0x1E06)
#define NFC_FLASH_CMD			(NFC_REG_BASE + 0x1E08)
#define NFC_CONFIG			(NFC_REG_BASE + 0x1E0A)
#define NFC_ECC_STATUS1			(NFC_REG_BASE + 0x1E0C)
#define NFC_ECC_STATUS2			(NFC_REG_BASE + 0x1E0E)
#define NFC_SPAS			(NFC_REG_BASE + 0x1E10)
#define NFC_WRPROT			(NFC_REG_BASE + 0x1E12)
#define NFC_NF_WRPRST			(NFC_REG_BASE + 0x1E18)
#define NFC_CONFIG1			(NFC_REG_BASE + 0x1E1A)
#define NFC_CONFIG2			(NFC_REG_BASE + 0x1E1C)
#define NFC_UNLOCKSTART_BLKADDR0	(NFC_REG_BASE + 0x1E20)
#define NFC_UNLOCKEND_BLKADDR0		(NFC_REG_BASE + 0x1E22)
#define NFC_UNLOCKSTART_BLKADDR1	(NFC_REG_BASE + 0x1E24)
#define NFC_UNLOCKEND_BLKADDR1		(NFC_REG_BASE + 0x1E26)
#define NFC_UNLOCKSTART_BLKADDR2	(NFC_REG_BASE + 0x1E28)
#define NFC_UNLOCKEND_BLKADDR2		(NFC_REG_BASE + 0x1E2A)
#define NFC_UNLOCKSTART_BLKADDR3	(NFC_REG_BASE + 0x1E2C)
#define NFC_UNLOCKEND_BLKADDR3		(NFC_REG_BASE + 0x1E2E)

/*!
 * Addresses for NFC MAIN RAM BUFFER areas
 */
#define MAIN_AREA(n)			(NFC_REG_BASE + (n)*0x200)

/*!
 * Addresses for NFC SPARE BUFFER areas
 */
#define SPARE_LEN			0x40
#define SPARE_AREA(n)			(NFC_REG_BASE + 0x1000 + (n)*SPARE_LEN)

#define NFC_CMD				0x1
#define NFC_ADDR			0x2
#define NFC_INPUT			0x4
#define NFC_OUTPUT			0x8
#define NFC_ID				0x10
#define NFC_STATUS			0x20

/* Bit Definitions */
#define NFC_INT				(1 << 15)
#define NFC_SP_EN			(1 << 2)
#define NFC_ECC_EN			(1 << 3)
#define NFC_INT_MSK			(1 << 4)
#define NFC_BIG				(1 << 5)
#define NFC_RST				(1 << 6)
#define NFC_CE				(1 << 7)
#define NFC_ONE_CYCLE			(1 << 8)
#define NFC_BLS_LOCKED			0
#define NFC_BLS_LOCKED_DEFAULT		1
#define NFC_BLS_UNLOCKED		2
#define NFC_WPC_LOCK_TIGHT		1
#define NFC_WPC_LOCK			(1 << 1)
#define NFC_WPC_UNLOCK			(1 << 2)
#define NFC_FLASH_ADDR_SHIFT 		0
#define NFC_UNLOCK_END_ADDR_SHIFT	0

#define NFC_ECC_MODE_4			1
/*
 * Define delays in microsec for NAND device operations
 */
#define TROP_US_DELAY			2000

#if defined(CONFIG_PPC)
#define NFC_WRITEL(r, v)		out_be32(r, v)
#define NFC_WRITEW(r, v)		out_be16(r, v)
#define NFC_WRITEB(r, v)		out_8(r, v)
#define NFC_READL(r)			in_be32(r)
#define NFC_READW(r)			in_be16(r)
#define NFC_READB(r)			in_8(r)
#elif defined(CONFIG_ARM)
#define NFC_WRITEL(r, v)		writel(v, r)
#define NFC_WRITEW(r, v)		writew(v, r)
#define NFC_WRITEB(r, v)		writeb(r, v)
#define NFC_READL(r)			readl(r)
#define NFC_READW(r)			readw(r)
#define NFC_READB(r)			readb(r)
#endif


#ifdef CONFIG_MTD_NAND_FSL_NFC_SWECC
static int hardware_ecc;
#else
static int hardware_ecc = 1;
#endif

/*
 * OOB placement block for use with hardware ecc generation
 */
static struct nand_ecclayout nand_hw_eccoob_512 = {
	.eccbytes = 9,
	.eccpos = {
		7, 8, 9, 10, 11, 12, 13, 14, 15,
	},
	.oobfree = {
		{0, 5} /* byte 5 is factory bad block marker */
	},
};

static struct nand_ecclayout nand_hw_eccoob_2k = {
	.eccbytes = 36,
	.eccpos = {
		/* 9 bytes of ecc for each 512 bytes of data */
		7, 8, 9, 10, 11, 12, 13, 14, 15,
		23, 24, 25, 26, 27, 28, 29, 30, 31,
		39, 40, 41, 42, 43, 44, 45, 46, 47,
		55, 56, 57, 58, 59, 60, 61, 62, 63,
	},
	.oobfree = {
		{2, 5}, /* bytes 0 and 1 are factory bad block markers */
		{16, 7},
		{32, 7},
		{48, 7},
	},
};

static struct nand_ecclayout nand_hw_eccoob_4k = {
	.eccbytes = 64,	/* actually 72 but only room for 64 */
	.eccpos = {
		/* 9 bytes of ecc for each 512 bytes of data */
		7, 8, 9, 10, 11, 12, 13, 14, 15,
		23, 24, 25, 26, 27, 28, 29, 30, 31,
		39, 40, 41, 42, 43, 44, 45, 46, 47,
		55, 56, 57, 58, 59, 60, 61, 62, 63,
		71, 72, 73, 74, 75, 76, 77, 78, 79,
		87, 88, 89, 90, 91, 92, 93, 94, 95,
		103, 104, 105, 106, 107, 108, 109, 110, 111,
		119, /* 120, 121, 122, 123, 124, 125, 126, 127, */
	},
	.oobfree = {
		{2, 5}, /* bytes 0 and 1 are factory bad block markers */
		{16, 7},
		{32, 7},
		{48, 7},
		{64, 7},
		{80, 7},
		{96, 7},
		{112, 7},
	},
};

static struct nand_ecclayout nand_hw_eccoob_4k_218_spare = {
	.eccbytes = 64,	/* actually 144 but only room for 64 */
	.eccpos = {
		/* 18 bytes of ecc for each 512 bytes of data */
		7, 8, 9, 10, 11, 12, 13, 14, 15,
		    16, 17, 18, 19, 20, 21, 22, 23, 24,
		33, 34, 35, 36, 37, 38, 39, 40, 41,
		    42, 43, 44, 45, 46, 47, 48, 49, 50,
		59, 60, 61, 62, 63, 64, 65, 66, 67,
		    68, 69, 70, 71, 72, 73, 74, 75, 76,
		85, 86, 87, 88, 89, 90, 91, 92, 93,
		    94, /* 95, 96, 97, 98, 99, 100, 101, 102,
		111, 112, 113, 114, 115, 116, 117, 118, 119,
		    120, 121, 122, 123, 124, 125, 126, 127, 128,
		137, 138, 139, 140, 141, 142, 143, 144, 145,
		    146, 147, 148, 149, 150, 151, 152, 153, 154,
		163, 164, 165, 166, 167, 168, 169, 170, 171,
		    172, 173, 174, 175, 176, 177, 178, 179, 180,
		189, 190, 191, 192, 193, 194, 195, 196, 197,
		    198, 199, 200, 201, 202, 203, 204, 205, 206, */
	},
	.oobfree = {
		{2, 5}, /* bytes 0 and 1 are factory bad block markers */
		{25, 8},
		{51, 8},
		{77, 8},
		{103, 8},
		{129, 8},
		{155, 8},
		{181, 8},
	},
};

/*
 * Functions to transfer data to/from spare erea.
 */
static void copy_from_spare(struct mtd_info *mtd, void *pbuf, int len)
{
	int i, copy_count, copy_size;

	copy_count = mtd->writesize / 512;
	/*
	 * Each spare area has 16 bytes for 512, 2K and normal 4K nand.
	 * For 4K nand with large 218 byte spare size, the size is 26 bytes for
	 * the first 7 buffers and 36 for the last.
	 */
	copy_size = priv->sparesize == 218 ? 26 : 16;

	for (i = 0; i < copy_count - 1 && len > 0; i++) {
		memcpy_fromio(pbuf, SPARE_AREA(i), MIN(len, copy_size));
		pbuf += copy_size;
		len -= copy_size;
	}
	if (len > 0)
		memcpy_fromio(pbuf, SPARE_AREA(i), len);
}

static void copy_to_spare(struct mtd_info *mtd, void *pbuf, int len)
{
	int i, copy_count, copy_size;

	copy_count = mtd->writesize / 512;
	/*
	 * Each spare area has 16 bytes for 512, 2K and normal 4K nand.
	 * For 4K nand with large 218 byte spare size, the size is 26 bytes for
	 * the first 7 buffers and 36 for the last.
	 */
	copy_size = priv->sparesize == 218 ? 26 : 16;

	/*
	 * Each spare area has 16 bytes for 512, 2K and normal 4K nand.
	 * For 4K nand with large 218 byte spare size, the size is 26 bytes for
	 * the first 7 buffers and 36 for the last.
	 */
	for (i = 0; i < copy_count - 1 && len > 0; i++) {
		memcpy_toio(SPARE_AREA(i), pbuf, MIN(len, copy_size));
		pbuf += copy_size;
		len -= copy_size;
	}
	if (len > 0)
		memcpy_toio(SPARE_AREA(i), pbuf, len);
}

/*!
 * This function polls the NFC to wait for the basic operation to complete by
 * checking the INT bit of config2 register.
 *
 * @max_retries	number of retry attempts (separated by 1 us)
 */
static void wait_op_done(int max_retries)
{

	while (1) {
		max_retries--;
		if (NFC_READW(NFC_CONFIG2) & NFC_INT)
			break;
		udelay(1);
	}
	if (max_retries <= 0)
		MTDDEBUG(MTD_DEBUG_LEVEL0, "%s: INT not set\n", __FUNCTION__);
}

/*!
 * This function issues the specified command to the NAND device and
 * waits for completion.
 *
 * @cmds	command for NAND Flash
 */
static void send_cmd(u16 cmd)
{
	MTDDEBUG(MTD_DEBUG_LEVEL3, "send_cmd(%#x)\n", cmd);

	NFC_WRITEW(NFC_FLASH_CMD, cmd);
	NFC_WRITEW(NFC_CONFIG2, NFC_CMD);

	/* Wait for operation to complete */
	wait_op_done(TROP_US_DELAY);
}

/*!
 * This function sends an address (or partial address) to the
 * NAND device.  The address is used to select the source/destination for
 * a NAND command.
 *
 * @addr	address to be written to NFC.
 */
static void send_addr(u16 addr)
{
	MTDDEBUG(MTD_DEBUG_LEVEL3, "send_addr(%#x)\n", addr);
	NFC_WRITEW(NFC_FLASH_ADDR, (addr << NFC_FLASH_ADDR_SHIFT));

	NFC_WRITEW(NFC_CONFIG2, NFC_ADDR);

	/* Wait for operation to complete */
	wait_op_done(TROP_US_DELAY);
}

/*!
 * This function requests the NFC to initate the transfer
 * of data currently in the NFC RAM buffer to the NAND device.
 *
 * @buf_id	Specify Internal RAM Buffer number (0-3)
 */
static void send_prog_page(u8 buf_id)
{
	u32 val = NFC_READW(NFC_BUF_ADDR);
	MTDDEBUG(MTD_DEBUG_LEVEL3, "%s\n", __FUNCTION__);

	/* Set RBA bits for BUFFER val */
	val &= ~0x7;
	val |= buf_id;
	NFC_WRITEW(NFC_BUF_ADDR, val);

	NFC_WRITEW(NFC_CONFIG2, NFC_INPUT);

	/* Wait for operation to complete */
	wait_op_done(TROP_US_DELAY);
}

/*!
 * This function requests the NFC to initated the transfer
 * of data from the NAND device into in the NFC ram buffer.
 *
 * @buf_id		Specify Internal RAM Buffer number (0-3)
 */
static void send_read_page(u8 buf_id)
{
	u32 val = NFC_READW(NFC_BUF_ADDR);
	MTDDEBUG(MTD_DEBUG_LEVEL3, "%s\n", __FUNCTION__);

	/* Set RBA bits for BUFFER val */
	val &= ~0x7;
	val |= buf_id;
	NFC_WRITEW(NFC_BUF_ADDR, val);

	NFC_WRITEW(NFC_CONFIG2, NFC_OUTPUT);

	/* Wait for operation to complete */
	wait_op_done(TROP_US_DELAY);
}

/*!
 * This function requests the NFC to perform a read of the
 * NAND device ID.
 */
static void send_read_id(void)
{
	u32 val = NFC_READW(NFC_BUF_ADDR);

	/* NFC buffer 0 is used for device ID output */
	/* Set RBA bits for BUFFER0 */
	val &= ~0x7;
	NFC_WRITEW(NFC_BUF_ADDR, val);

	/* Read ID into main buffer */
	NFC_WRITEW(NFC_CONFIG2, NFC_ID);

	/* Wait for operation to complete */
	wait_op_done(TROP_US_DELAY);

}

/*!
 * This function requests the NFC to perform a read of the
 * NAND device status and returns the current status.
 *
 * @return	device status
 */
static u16 get_dev_status(void)
{
	u32 save;
	u16 ret;
	u32 val;
	/* Issue status request to NAND device */

	/* save the main area1 first word, later do recovery */
	save = NFC_READL(MAIN_AREA(1));
	NFC_WRITEL(MAIN_AREA(1), 0);

	/*
	 * NFC buffer 1 is used for device status to prevent
	 * corruption of read/write buffer on status requests.
	 */

	/* Select BUFFER1 */
	val = NFC_READW(NFC_BUF_ADDR);
	val &= ~0x7;
	val |= 1;
	NFC_WRITEW(NFC_BUF_ADDR, val);

	/* Read status into main buffer */
	NFC_WRITEW(NFC_CONFIG2, NFC_STATUS);

	/* Wait for operation to complete */
	wait_op_done(TROP_US_DELAY);

	/* Status is placed in first word of main buffer */
	/* get status, then recovery area 1 data */
	if (NFC_READW(NFC_CONFIG1) & NFC_BIG)
		ret = NFC_READB(MAIN_AREA(1));
	else
		ret = NFC_READB(MAIN_AREA(1) + 3);

	NFC_WRITEL(MAIN_AREA(1), save);
	return ret;
}

/*!
 * This functions is used by upper layer to checks if device is ready
 *
 * @mtd		MTD structure for the NAND Flash
 *
 * @return	0 if device is busy else 1
 */
static int fsl_nfc_dev_ready(struct mtd_info *mtd)
{
	return 1;
}

static void fsl_nfc_enable_hwecc(struct mtd_info *mtd, int mode)
{
	NFC_WRITEW(NFC_CONFIG1, (NFC_READW(NFC_CONFIG1) | NFC_ECC_EN));
	return;
}

/*
 * Function to record the ECC corrected/uncorrected errors resulted
 * after a page read. This NFC detects and corrects upto to 4 symbols
 * of 9-bits each.
 */
static int fsl_nfc_check_ecc_status(struct mtd_info *mtd)
{
	u32 ecc_stat, err;
	int no_subpages = 1;
	int ret = 0;
	u8 ecc_bit_mask, err_limit;
	int is_4bit_ecc = NFC_READW(NFC_CONFIG1) & NFC_ECC_MODE_4;

	ecc_bit_mask = (is_4bit_ecc ? 0x7 : 0xf);
	err_limit = (is_4bit_ecc ? 0x4 : 0x8);

	no_subpages = mtd->writesize >> 9;

	ecc_stat = NFC_READW(NFC_ECC_STATUS1);
	do {
		err = ecc_stat & ecc_bit_mask;
		if (err > err_limit)
			return -1;
		else
			ret += err;
		ecc_stat >>= 4;
	} while (--no_subpages);

	return ret;
}

/*!
 * This function reads byte from the NAND Flash
 *
 * @mtd		MTD structure for the NAND Flash
 *
 * @return	data read from the NAND Flash
 */
static u_char fsl_nfc_read_byte(struct mtd_info *mtd)
{
	void *area_buf;
	u_char rv;

	/* Check for status request */
	if (priv->status_req) {
		rv = get_dev_status() & 0xff;
		return rv;
	}

	if (priv->spare_only)
		area_buf = SPARE_AREA(0);
	else
		area_buf = MAIN_AREA(0);

	rv = NFC_READB(area_buf + priv->col_addr);
	priv->col_addr++;
	return rv;
}

/*!
  * This function reads word from the NAND Flash
  *
  * @mtd	MTD structure for the NAND Flash
  *
  * @return	data read from the NAND Flash
  */
static u16 fsl_nfc_read_word(struct mtd_info *mtd)
{
	u16 rv;
	void *area_buf;

	/* If we are accessing the spare region */
	if (priv->spare_only)
		area_buf = SPARE_AREA(0);
	else
		area_buf = MAIN_AREA(0);

	/* Update saved column address */
	rv = NFC_READW(area_buf + priv->col_addr);
	priv->col_addr += 2;

	return rv;
}

/*!
 * This function reads byte from the NAND Flash
 *
 * @mtd		MTD structure for the NAND Flash
 *
 * @return	data read from the NAND Flash
 */
static u_char fsl_nfc_read_byte16(struct mtd_info *mtd)
{
	/* Check for status request */
	if (priv->status_req)
		return (get_dev_status() & 0xff);

	return fsl_nfc_read_word(mtd) & 0xff;
}

/*!
 * This function writes data of length \b len from buffer \b buf to the NAND
 * internal RAM buffer's MAIN area 0.
 *
 * @mtd		MTD structure for the NAND Flash
 * @buf		data to be written to NAND Flash
 * @len		number of bytes to be written
 */
static void fsl_nfc_write_buf(struct mtd_info *mtd,
					const u_char *buf, int len)
{
	if (priv->col_addr >= mtd->writesize || priv->spare_only) {
		copy_to_spare(mtd, (char *)buf, len);
		return;
	} else {
		priv->col_addr += len;
		memcpy_toio(MAIN_AREA(0), (void *)buf, len);
	}
}

/*!
 * This function id is used to read the data buffer from the NAND Flash. To
 * read the data from NAND Flash first the data output cycle is initiated by
 * the NFC, which copies the data to RAMbuffer. This data of length \b len is
 * then copied to buffer \b buf.
 *
 * @mtd		MTD structure for the NAND Flash
 * @buf		data to be read from NAND Flash
 * @len		number of bytes to be read
 */
static void fsl_nfc_read_buf(struct mtd_info *mtd, u_char *buf, int len)
{

	if (priv->col_addr >= mtd->writesize || priv->spare_only) {
		copy_from_spare(mtd, buf, len);
		return;
	} else {
		priv->col_addr += len;
		memcpy_fromio((void *)buf, MAIN_AREA(0), len);
	}
}

/*!
 * This function is used by the upper layer to verify the data in NAND Flash
 * with the data in the \b buf.
 *
 * @mtd		MTD structure for the NAND Flash
 * @buf		data to be verified
 * @len		length of the data to be verified
 *
 * @return	-1 if error else 0
 *
 */
static int fsl_nfc_verify_buf(struct mtd_info *mtd, const u_char *buf,
					int len)
{
	void *main_buf = MAIN_AREA(0);
	/* check for 32-bit alignment? */
	u32 *p = (u32 *) buf;
	u32 v = 0;

	for (; len > 0; len -= 4, main_buf += 4)
		v = NFC_READL(main_buf);
		if (v != *p++)
			return -1;
	return 0;
}

static int fsl_nfc_get_hw_config(struct nand_chip *this)
{
	immap_t *im = (immap_t *)CONFIG_SYS_IMMR;
	u32 rcwh;
	int rcwh_romloc;
	int rcwh_ps;
	int width;
	int writesize = 0;
	int sparesize = 0;

	/*
	 * Only support 2K for now.
	 * Remove this when others are tested and debugged.
	 */
#if 1
	if (CONFIG_FSL_NFC_WRITE_SIZE != 2048) {
		printf("FSL NFC: "
			"%d byte write size flash support is untested\n",
			CONFIG_FSL_NFC_WRITE_SIZE);
		return -1;
	}
#endif
	rcwh = NFC_READL((void *)&(im->reset.rcwh));
	width = ((rcwh >> 6) & 0x1) ? 2 : 1;

	if (width != CONFIG_FSL_NFC_WIDTH) {
		printf("FSL NFC: Device width mismatch, compiled for %d, "
			"reset configuration word width is %d\n",
			CONFIG_FSL_NFC_WIDTH, width);
		return -1;
	}

	if (width == 2) {
		this->options |= NAND_BUSWIDTH_16;
		this->read_byte = fsl_nfc_read_byte16;
	}

	/*
	 * Decode the rcwh_ps and rcwh_romloc
	 * bits from reset config word
	 * to determine write size
	 */
	rcwh_ps = (rcwh >> 7) & 0x1;
	rcwh_romloc = (rcwh >> 21) & 0x3;
	switch (rcwh_ps << 2 | rcwh_romloc) {
	case 0x0:
	case 0x1:
		writesize = 512;
		sparesize = 16;
		break;
	case 0x2:
	case 0x3:
		writesize = 4096;
		sparesize = 128;
		break;
	case 0x4:
	case 0x5:
		writesize = 2048;
		sparesize = 64;
		break;
	case 0x6:
	case 0x7:
		writesize = 4096;
		sparesize = 218;
		break;
	}
	if (CONFIG_FSL_NFC_WRITE_SIZE != writesize) {
		printf("FSL NFC: "
			"Device write size mismatch, "
			"compiled for %d, "
			"size from reset configuration word is %d\n",
			CONFIG_FSL_NFC_WRITE_SIZE, writesize);
		return -1;
	}
	if (CONFIG_FSL_NFC_SPARE_SIZE != sparesize) {
		printf("FSL NFC: "
			"Device spare size mismatch, "
			"compiled for %d, "
			"size from reset configuration word is %d\n",
			CONFIG_FSL_NFC_SPARE_SIZE, sparesize);
		return -1;
	}

	priv->sparesize = sparesize;
	priv->writesize = writesize;
	priv->width = width;
	return 0;
}


#ifndef CONFIG_FSL_NFC_BOARD_CS_FUNC
static void fsl_nfc_select_chip(u8 cs)
{
	u32 val = NFC_READW(NFC_BUF_ADDR);

	val &= ~0x60;
	val |= cs << 5;
	NFC_WRITEW(NFC_BUF_ADDR, val);
}
#define CONFIG_FSL_NFC_BOARD_CS_FUNC fsl_nfc_select_chip
#endif


/*!
 * This function is used by upper layer for select and deselect of the NAND
 * chip
 *
 * @mtd		MTD structure for the NAND Flash
 * @chip	val indicating select or deselect
 */
static void fsl_nfc_select_chip(struct mtd_info *mtd, int chip)
{
	/*
	 * This is different than the linux version.
	 * Switching between chips is done via
	 * board_nand_select_device.
	 *
	 * Only valid chip numbers here are
	 * 	0 select
	 * 	-1 deselect
	 */
	if (chip < -1 || chip > 0) {
		printf("FSL NFC: "
			"ERROR: Illegal chip select (chip = %d)\n", chip);
	}

	if (chip < 0) {
		NFC_WRITEW(NFC_CONFIG1, (NFC_READW(NFC_CONFIG1) & ~NFC_CE));
		return;
	}

	NFC_WRITEW(NFC_CONFIG1, (NFC_READW(NFC_CONFIG1) | NFC_CE));

	/*
	 * Turn on appropriate chip.
	 */
	CONFIG_FSL_NFC_BOARD_CS_FUNC(priv->chipsel);
}

/*
 * Function to perform the address cycles.
 */
static void fsl_nfc_do_addr_cycle(struct mtd_info *mtd, int column,
		int page_addr)
{
	struct nand_chip *this = mtd->priv;
	u32 page_mask = this->pagemask;

	if (column != -1) {
		send_addr(column & 0xff);
		/* large page nand needs an extra column addr cycle */
		if (IS_2K_PAGE_NAND)
			send_addr((column >> 8) & 0xf);
		else if (IS_4K_PAGE_NAND)
			send_addr((column >> 8) & 0x1f);
	}
	if (page_addr != -1) {
		do {
			send_addr((page_addr & 0xff));
			page_mask >>= 8;
			page_addr >>= 8;
		} while (page_mask != 0);
	}
}

/*
 * Function to read a page from nand device.
 */
static void read_full_page(struct mtd_info *mtd, int page_addr)
{
	send_cmd(NAND_CMD_READ0);

	fsl_nfc_do_addr_cycle(mtd, 0, page_addr);

	if (IS_LARGE_PAGE_NAND) {
		send_cmd(NAND_CMD_READSTART);
		send_read_page(0);
	} else {
		send_read_page(0);
	}
}

/*!
 * This function is used by the upper layer to write command to NAND Flash for
 * different operations to be carried out on NAND Flash
 *
 * @mtd		MTD structure for the NAND Flash
 * @command	command for NAND Flash
 * @column	column offset for the page read
 * @page_addr	page to be read from NAND Flash
 */
static void fsl_nfc_command(struct mtd_info *mtd, unsigned command,
					int column, int page_addr)
{
	MTDDEBUG(MTD_DEBUG_LEVEL3,
		"fsl_nfc_command (cmd = %#x, col = %#x, page = %#x)\n",
		command, column, page_addr);
	/*
	 * Reset command state information
	 */
	priv->status_req = 0;

	/* Reset column address to 0 */
	priv->col_addr = 0;

	/*
	 * Command pre-processing step
	 */
	switch (command) {
	case NAND_CMD_STATUS:
		priv->status_req = 1;
		break;

	case NAND_CMD_READ0:
		priv->spare_only = 0;
		break;

	case NAND_CMD_READOOB:
		priv->col_addr = column;
		priv->spare_only = 1;
		command = NAND_CMD_READ0;	/* only READ0 is valid */
		break;

	case NAND_CMD_SEQIN:
		if (column >= mtd->writesize)
			priv->spare_only = 1;
		else
			priv->spare_only = 0;
		break;

	case NAND_CMD_PAGEPROG:
		if (!priv->spare_only)
			send_prog_page(0);
		else
			return;
		break;

	case NAND_CMD_ERASE1:
		break;
	case NAND_CMD_ERASE2:
		break;
	}

	/*
	 * Write out the command to the device.
	 */
	send_cmd(command);

	fsl_nfc_do_addr_cycle(mtd, column, page_addr);

	/*
	 * Command post-processing step
	 */
	switch (command) {

	case NAND_CMD_READOOB:
	case NAND_CMD_READ0:
		if (IS_LARGE_PAGE_NAND) {
			/* send read confirm command */
			send_cmd(NAND_CMD_READSTART);
			/* read for each AREA */
			send_read_page(0);
		} else
			send_read_page(0);
		break;

	case NAND_CMD_READID:
		send_read_id();
		break;
	}
}

static int fsl_nfc_wait(struct mtd_info *mtd, struct nand_chip *chip)
{
	return get_dev_status();
}

static int fsl_nfc_read_oob(struct mtd_info *mtd, struct nand_chip *chip,
				int page, int sndcmd)
{
	if (sndcmd) {
		read_full_page(mtd, page);
		sndcmd = 0;
	}

	copy_from_spare(mtd, chip->oob_poi, mtd->oobsize);
	return sndcmd;
}

static int fsl_nfc_write_oob(struct mtd_info *mtd, struct nand_chip *chip,
					int page)
{
	int status = 0;
	int read_oob_col = 0;

	send_cmd(NAND_CMD_READ0);
	send_cmd(NAND_CMD_SEQIN);
	fsl_nfc_do_addr_cycle(mtd, read_oob_col, page);

	/* copy the oob data */
	copy_to_spare(mtd, chip->oob_poi, mtd->oobsize);

	send_prog_page(0);

	send_cmd(NAND_CMD_PAGEPROG);

	status = fsl_nfc_wait(mtd, chip);
	if (status & NAND_STATUS_FAIL)
		return -1;
	return 0;
}

static int fsl_nfc_read_page(struct mtd_info *mtd, struct nand_chip *chip,
					uint8_t *buf)
{
	int stat;

	stat = fsl_nfc_check_ecc_status(mtd);
	if (stat == -1) {
		mtd->ecc_stats.failed++;
		printf("FSL NFC: UnCorrectable RS-ECC Error\n");
	} else {
		mtd->ecc_stats.corrected += stat;
		if (stat)
			printf("%d Symbol Correctable RS-ECC Error\n", stat);
	}

	memcpy_fromio((void *)buf, MAIN_AREA(0), mtd->writesize);
	copy_from_spare(mtd, chip->oob_poi, mtd->oobsize);
	return 0;
}

static void fsl_nfc_write_page(struct mtd_info *mtd,
		struct nand_chip *chip, const uint8_t *buf)
{
	memcpy_toio(MAIN_AREA(0), buf, mtd->writesize);
	copy_to_spare(mtd, chip->oob_poi, mtd->oobsize);
}


/*
 * Generic flash bbt decriptors
 */
static uint8_t bbt_pattern[] = { 'B', 'b', 't', '0' };
static uint8_t mirror_pattern[] = { '1', 't', 'b', 'B' };

/*
 * These are identical to the generic versions except
 * for the offsets.
 */
static struct nand_bbt_descr bbt_main_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
	    | NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs = 0,
	.len = 4,
	.veroffs = 4,
	.maxblocks = 4,
	.pattern = bbt_pattern
};

static struct nand_bbt_descr bbt_mirror_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
	    | NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs = 0,
	.len = 4,
	.veroffs = 4,
	.maxblocks = 4,
	.pattern = mirror_pattern
};

void board_nand_select_device(struct nand_chip *nand, int chip)
{
	if (chip >= CONFIG_FSL_NFC_CHIPS) {
		printf("FSL NFC: "
			"ERROR: Illegal chip select (chip = %d)\n", chip);
		return;
	}
	priv->chipsel = chip;
}


int board_nand_init(struct nand_chip *nand)
{
	struct mtd_info *mtd;

	priv = malloc(sizeof(*priv));
	if (!priv) {
		printf("FSL NFC: failed to allocate priv structure\n");
		return -1;
	}
	memset(priv, 0, sizeof(*priv));

	if (fsl_nfc_get_hw_config(nand) < 0)
		return -1;

	mtd = &priv->mtd;
	mtd->priv = nand;

	/* 5 us command delay time */
	nand->chip_delay = 5;

	nand->dev_ready = fsl_nfc_dev_ready;
	nand->cmdfunc = fsl_nfc_command;
	nand->waitfunc = fsl_nfc_wait;
	nand->select_chip = fsl_nfc_select_chip;
	nand->options = NAND_USE_FLASH_BBT;
	if (priv->width == 2) {
		nand->options |= NAND_BUSWIDTH_16;
		nand->read_byte = fsl_nfc_read_byte16;
	}
	nand->read_byte = fsl_nfc_read_byte;
	nand->read_word = fsl_nfc_read_word;
	nand->write_buf = fsl_nfc_write_buf;
	nand->read_buf = fsl_nfc_read_buf;
	nand->verify_buf = fsl_nfc_verify_buf;

	nand->bbt_td = &bbt_main_descr;
	nand->bbt_md = &bbt_mirror_descr;

	NFC_WRITEW(NFC_CONFIG1, (NFC_READW(NFC_CONFIG1) | NFC_RST));

	/* Disable interrupt */
	NFC_WRITEW(NFC_CONFIG1, (NFC_READW(NFC_CONFIG1) | NFC_INT_MSK));

	if (hardware_ecc) {
		nand->ecc.read_page = fsl_nfc_read_page;
		nand->ecc.write_page = fsl_nfc_write_page;
		nand->ecc.read_oob = fsl_nfc_read_oob;
		nand->ecc.write_oob = fsl_nfc_write_oob;
		if (IS_2K_PAGE_NAND)
			nand->ecc.layout = &nand_hw_eccoob_2k;
		else if (IS_4K_PAGE_NAND)
			if (priv->sparesize == 128)
				nand->ecc.layout = &nand_hw_eccoob_4k;
			else
				nand->ecc.layout = &nand_hw_eccoob_4k_218_spare;
		else
			nand->ecc.layout = &nand_hw_eccoob_512;
		/* propagate ecc.layout to mtd_info */
		mtd->ecclayout = nand->ecc.layout;
		nand->ecc.calculate = NULL;
		nand->ecc.hwctl = fsl_nfc_enable_hwecc;
		nand->ecc.correct = NULL;
		nand->ecc.mode = NAND_ECC_HW;
		/* RS-ECC is applied for both MAIN+SPARE not MAIN alone */
		nand->ecc.size = 512;
		nand->ecc.bytes = 9;
		NFC_WRITEW(NFC_CONFIG1, (NFC_READW(NFC_CONFIG1) | NFC_ECC_EN));
	} else {
		nand->ecc.mode = NAND_ECC_SOFT;
		NFC_WRITEW(NFC_CONFIG1, (NFC_READW(NFC_CONFIG1) & ~NFC_ECC_EN));
	}

	NFC_WRITEW(NFC_CONFIG1, NFC_READW(NFC_CONFIG1) & ~NFC_SP_EN);


	/* Reset NAND */
	nand->cmdfunc(mtd, NAND_CMD_RESET, -1, -1);

	/* preset operation */
	/* Unlock the internal RAM Buffer */
	NFC_WRITEW(NFC_CONFIG, NFC_BLS_UNLOCKED);

	/* Blocks to be unlocked */
	NFC_WRITEW(NFC_UNLOCKSTART_BLKADDR0, 0x0);
	NFC_WRITEW(NFC_UNLOCKEND_BLKADDR0, 0xffff);

	/* Unlock Block Command for given address range */
	NFC_WRITEW(NFC_WRPROT, NFC_WPC_UNLOCK);

	/* Set sparesize */
	NFC_WRITEW(NFC_SPAS,
		(NFC_READW(NFC_SPAS) & 0xff00) | (priv->sparesize/2));

	/*
	 * Only use 8bit ecc (aka not 4 bit) if large spare size
	 */
	if (priv->sparesize == 218)
		NFC_WRITEW(NFC_CONFIG1,
			(NFC_READW(NFC_CONFIG1) & ~NFC_ECC_MODE_4));
	else
		NFC_WRITEW(NFC_CONFIG1,
			(NFC_READW(NFC_CONFIG1) | NFC_ECC_MODE_4));

	return 0;
}
