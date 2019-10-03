#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/regulator/consumer.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <mach/gpio.h>
#include "mcu_data_misc.h"

#define DEV_NAME "fast_car_reverse"
#define REVERSE_IOC_MAGIC  'z'


#define MCU_DATA_P 1
//#ifdef MCU_DATA_P

//#else
/*
struct mcu_data_t{
      unsigned int type;
      unsigned int addr;
      unsigned int len;
      unsigned int smd;
      unsigned int *prv;
};
*/
//#endif

#define REVERSE_IOC_TEST_DATA  _IOW(REVERSE_IOC_MAGIC, 1, unsigned int *)
#define REVERSE_IOC_MCU_DATA  _IOW(REVERSE_IOC_MAGIC, 2, struct mcu_data_t *)
#define REVERSE_IOC_RMCU_DATA  _IOR(REVERSE_IOC_MAGIC, 3, struct mcu_data_t *)


struct mcu_data_t *mcu_data_prv;
struct semaphore mcu_wakesem;

EXPORT_SYMBOL(mcu_data_prv);

static int mcu_data_open(struct inode *inode, struct file *filp)
{
    if (mcu_data_prv == NULL)
        return -ENODEV;

	filp->private_data = mcu_data_prv;

	return 0;
}

static int mcu_data_release(struct inode *inode, struct file *filp)
{
	filp->private_data = NULL;

	return 0;
}

#ifdef MCU_DATA_P
static long mcu_data_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int err = 0;
	void __user *argp = (void __user *)arg;
	int value0;
	int value1 ;
	
	/* Verify user arguments. */
	//if (_IOC_TYPE(cmd) != REVERSE_IOC_MCU_DATA)
		//return -ENOTTY;
    printk("mcu_data_ioctl Enter\n");
	switch(cmd) {
	printk("mcu_data_ioctl Enter\n");
	case REVERSE_IOC_TEST_DATA:
		printk("mcu_data_TEST\n");
		if (copy_from_user(&value0, argp, sizeof(int))) {
			printk("copy_from_user error\r\n");
			//return -EFAULT;
		}
		
	    printk("value0 = %d\r\n",value0);
		break;
	case REVERSE_IOC_MCU_DATA:
		printk("mcu_data_WRITE\n");
		if (copy_from_user(mcu_data_prv, argp, sizeof(struct mcu_data_t))) {
			printk("copy_from_user error\r\n");
			//return -EFAULT;
		}
		printk("this is user(mcu) data:  11111\r\n");
		printk("actual_gear=%d\n",mcu_data_prv->actual_gear);
		printk("steering_angle=%d\n",mcu_data_prv->steering_angle);
		printk("r_radar_distance=%d\n",mcu_data_prv->r_radar_distance);
		printk("m_radar_distance=%d\n",mcu_data_prv->m_radar_distance);
		printk("l_radar_distance=%d\n",mcu_data_prv->l_radar_distance);
        up(&mcu_wakesem);
		break;
#if 1
	case REVERSE_IOC_RMCU_DATA:
		printk("mcu_data_READ\n");
		mcu_data_prv->actual_gear=2;
		mcu_data_prv->steering_angle=2;
		mcu_data_prv->r_radar_distance=2;
		mcu_data_prv->m_radar_distance=2;
		mcu_data_prv->l_radar_distance=2;
		if (copy_to_user(argp, mcu_data_prv, sizeof(struct mcu_data_t))) {
			printk("copy_to_user error\r\n");
			return -EFAULT;
		}

		break;
#endif
	default:
		printk("Invalid ioctl command.\n");
		return -ENOTTY;
	}

	return err;
}

#else
static long mcu_data_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int err = 0;
	void __user *argp = (void __user *)arg;
	int value0;
	int value1 ;
	
	/* Verify user arguments. */
	//if (_IOC_TYPE(cmd) != REVERSE_IOC_MCU_DATA)
		//return -ENOTTY;

	struct mcu_data_t tmp;
	struct mcu_data_t *ptmp = &tmp;
	unsigned int *pbuff_ram1; //预先定义一个指针

	printk("mcu_data_ioctl Enter\n");
	switch(cmd) {
	printk("mcu_data_ioctl Enter\n");
	case REVERSE_IOC_TEST_DATA:
		printk("mcu_data_TEST\n");
		if (copy_from_user(&value0, argp, sizeof(int))) {
			printk("copy_from_user error\r\n");
			//return -EFAULT;
		}
		
	    printk("value0 = %d\r\n",value0);
		break;
	case REVERSE_IOC_MCU_DATA:
		printk("mcu_data_WRITE\n");
		if (copy_from_user(mcu_data_prv, argp, sizeof(struct mcu_data_t)))
			//return -EFAULT;
		printk("mcu_data_prv->type=%d\n",mcu_data_prv->type);
		printk("mcu_data_prv->addr=%d\n",mcu_data_prv->addr);
		printk("mcu_data_prv->smd=%d\n",mcu_data_prv->smd);

		copy_from_user((unsigned int *)ptmp,(unsigned int *)argp,sizeof(struct mcu_data_t));
		pbuff_ram1 = (unsigned int *)kmalloc(6*sizeof(unsigned int),GFP_KERNEL);
		copy_from_user((unsigned int *)pbuff_ram1,(unsigned int *)(( (mcu_data_t *)ptmp)->prv),6*sizeof(unsigned int));
		for(i = 0; i < 6; i++) {  
	    	printk("0x%04x ",*((unsigned int *)(pbuff_ram1 +i)));    
		} 
		
        kfree(pbuff_ram1);
		break;
#if 1
	case REVERSE_IOC_RMCU_DATA:
		printk("mcu_data_READ\n");
		mcu_data_prv->type=1;
		mcu_data_prv->addr=2;
		mcu_data_prv->smd=3;
		if (copy_to_user(argp, mcu_data_prv, sizeof(struct mcu_data_t))) {
			printk("copy_to_user error\r\n");
			return -EFAULT;
		}

		break;
#endif
	default:
		printk("Invalid ioctl command.\n");
		return -ENOTTY;
	}

	return err;
}
#endif

static long mcu_data_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int err = 0;
	void __user *argp = (void __user *)arg;
	int value0;
	int value1 ;
	
	/* Verify user arguments. */
	//if (_IOC_TYPE(cmd) != REVERSE_IOC_MCU_DATA)
		//return -ENOTTY;

    printk("mcu_data_compat_ioctl Enter\n");
	switch(cmd) {
	printk("mcu_data_compat_ioctl Enter\n");
	case REVERSE_IOC_TEST_DATA:
		printk("mcu_data_TEST\n");
		if (copy_from_user(&value0, argp, sizeof(int))) {
			printk("copy_from_user error\r\n");
			//return -EFAULT;
		}
		
	    printk("value0 = %d\r\n",value0);
		break;
	case REVERSE_IOC_MCU_DATA:
		printk("mcu_data_WRITE\n");
		if (copy_from_user(mcu_data_prv, argp, sizeof(struct mcu_data_t))) {
			printk("copy_from_user error\r\n");
			//return -EFAULT;
		}
		printk("this is user(mcu) data:  11111\r\n");
		printk("actual_gear=%d\n",mcu_data_prv->actual_gear);
		printk("steering_angle=%d\n",mcu_data_prv->steering_angle);
		printk("r_radar_distance=%d\n",mcu_data_prv->r_radar_distance);
		printk("m_radar_distance=%d\n",mcu_data_prv->m_radar_distance);
		printk("l_radar_distance=%d\n",mcu_data_prv->l_radar_distance);
        up(&mcu_wakesem);
		break;
#if 1
	case REVERSE_IOC_RMCU_DATA:
		printk("mcu_data_READ\n");
		mcu_data_prv->actual_gear=2;
		mcu_data_prv->steering_angle=2;
		mcu_data_prv->r_radar_distance=2;
		mcu_data_prv->m_radar_distance=2;
		mcu_data_prv->l_radar_distance=2;
		if (copy_to_user(argp, mcu_data_prv, sizeof(struct mcu_data_t))) {
			printk("copy_to_user error\r\n");
			return -EFAULT;
		}
		
		break;
#endif
	default:
		printk("Invalid ioctl command.\n");
		return -ENOTTY;
	}

	return err;
}



static const struct file_operations mcu_data_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = mcu_data_ioctl,
	.compat_ioctl = mcu_data_compat_ioctl,
	.open = mcu_data_open,
	.release = mcu_data_release
};

static struct miscdevice mcu_data_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEV_NAME,
	.fops = &mcu_data_fops
};

static int mcu_data_probe(struct platform_device *pdev)
{
	int ret = 0;

	printk("mcu_data_probe 0\n");
	/* Initialize */
	mcu_data_prv = kzalloc(sizeof(struct mcu_data_t), GFP_KERNEL);

	if (mcu_data_prv == NULL) {
		printk("Not enough memory to initialize device\n");
		return -ENOMEM;
    }

	ret = misc_register(&mcu_data_dev);
   
	if (ret < 0)
		goto err;

	sema_init(&mcu_wakesem,0);
    printk("%s Exit\r\n",__FUNCTION__);
	return 0;

err:
	printk("mcu_data register failed\n");
	kfree(mcu_data_prv);
	mcu_data_prv = NULL;

	return ret;
}

static int  mcu_data_remove(struct platform_device *plat)
{
	kfree(mcu_data_prv);
	mcu_data_prv = NULL;
	misc_deregister(&mcu_data_dev);
	printk("mcu_data remove\n");

	return 0;
}

static const struct of_device_id mcu_data_dt_ids[] = {
    {.compatible = "allwinner,mcu_data"}, {},
};


static struct platform_driver mcu_data_driver = {
	.probe = mcu_data_probe,
	.remove = mcu_data_remove,
	.driver = {
	.name = DEV_NAME,
	.owner = THIS_MODULE,
	.of_match_table = mcu_data_dt_ids,
	},
};

static int __init mcu_data_init(void)
{
    int ret = 0;
	
    printk("mcu_data_init \n");
	ret = platform_driver_register(&mcu_data_driver);
    return ret;
}

static void __exit mcu_data_exit(void)
{
	platform_driver_unregister(&mcu_data_driver);
}

//module_init(mcu_data_init);
subsys_initcall_sync(mcu_data_init);
module_exit(mcu_data_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("chm <chm@chinatsp.com>");
MODULE_DESCRIPTION("fast car reverse get mcu data");
