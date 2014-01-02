#include <linux/module.h> /* Specifically, a module */
#include <linux/kernel.h> /* We're doing kernel work */
#include <linux/proc_fs.h>        /* Necessary because we use the proc fs */
#include <asm/uaccess.h>  /* for copy_from_user */

#include <linux/slab.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/fsl_devices.h>
#include <linux/spi/spi.h>
#include <linux/i2c.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>
#include <linux/regulator/consumer.h>
#include <linux/pmic_external.h>
#include <linux/pmic_status.h>
#include <linux/ipu.h>
#include <linux/mxcfb.h>
#include <linux/pwm_backlight.h>
#include <mach/common.h>
#include <mach/hardware.h>
#include <asm/setup.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/time.h>
#include <asm/mach/keypad.h>
#include <asm/mach/flash.h>
#include <mach/gpio.h>
#include <mach/mmc.h>
#include <mach/mxc_dvfs.h>
#include <mach/mxc_edid.h>
#include <mach/iomux-mx51.h>
#include <mach/i2c.h>

#include "devices.h"
#include "crm_regs.h"
#include "usb.h"
#include "mx51_pins_my.h"




#define PROCFS_MAX_SIZE         1024
#define PROCFS_NAME             "led35"
static int ledon=0;
#define BABBAGE_DVI_RESET		(2*32 + 5)	/* GPIO_3_5 */

/**
 * This structure hold information about the /proc file
 *
 */
static struct proc_dir_entry *Our_Proc_File;

/**
 * The buffer used to store character for this module
 *
 */
static char procfs_buffer[PROCFS_MAX_SIZE];

/**
 * The size of the buffer
 *
 */
static unsigned long procfs_buffer_size = 0;

/** 
 * This function is called then the /proc file is read
 *
 */
int 
procfile_read(char *buffer,
              char **buffer_location,
              off_t offset, int buffer_length, int *eof, void *data)
{
        int ret;
        
        //printk(KERN_INFO "procfile_read (/proc/%s) called\n", PROCFS_NAME);
        
        if (offset > 0) {
                /* we have finished to read, return 0 */
                ret  = 0;
        } else {
                /* fill the buffer, return the buffer size */
                memcpy(buffer, procfs_buffer, 2/*procfs_buffer_size*/);
                ret = 2/*procfs_buffer_size*/;
        }

        return ret;
}

/**
 * This function is called with the /proc file is written
 *
 */
int procfile_write(struct file *file, const char *buffer, unsigned long count,
                   void *data)
{
        /* get buffer size */
        procfs_buffer_size = count;
        if (procfs_buffer_size > PROCFS_MAX_SIZE ) {
                procfs_buffer_size = PROCFS_MAX_SIZE;
        }
        //procfs_buffer_size = 1;
        
        /* write data to the buffer */
        if ( copy_from_user(procfs_buffer, buffer, procfs_buffer_size ) ) {
                return -EFAULT;
        }
		if( procfs_buffer[0] == '0' ) ledon=0;
		else ledon=1;
		procfs_buffer[1]=0x0a;
		procfs_buffer[2]=0;
        //printk(KERN_INFO "procfile_write (/proc/%s) called 0x%02x count:%d\n", PROCFS_NAME, 0x0ff & procfs_buffer[0],count);

		gpio_set_value(BABBAGE_DVI_RESET, ledon);
		//ledon = (ledon+1)&1;
        
        return procfs_buffer_size;
}

/**
 *This function is called when the module is loaded
 *
 */
int init_module()
{
        /* create the /proc file */
        Our_Proc_File = create_proc_entry(PROCFS_NAME, 0644, NULL);
        
        if (Our_Proc_File == NULL) {
                remove_proc_entry(PROCFS_NAME, NULL);
                printk(KERN_ALERT "Error: Could not initialize /proc/%s\n",
                        PROCFS_NAME);
                return -ENOMEM;
        }

        Our_Proc_File->read_proc  = procfile_read;
        Our_Proc_File->write_proc = procfile_write;
        //Our_Proc_File->owner           = THIS_MODULE;
        Our_Proc_File->mode    = S_IFREG | S_IRUGO;
        Our_Proc_File->uid     = 0;
        Our_Proc_File->gid     = 0;
        Our_Proc_File->size    = 37;

        printk(KERN_INFO "/proc/%s created\n", PROCFS_NAME);    
        return 0;       /* everything is ok */
}

/**
 *This function is called when the module is unloaded
 *
 */
void cleanup_module()
{
        remove_proc_entry(PROCFS_NAME, NULL);
        printk(KERN_INFO "/proc/%s removed\n", PROCFS_NAME);
}

module_init(init_module);
module_exit(cleanup_module);