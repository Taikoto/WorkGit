#include <linux/module.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/stat.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <linux/kmod.h>
#include <linux/gfp.h>
#include <linux/gpio/consumer.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/slab.h>


struct gpio_key{
	int gpio;
	struct gpio_desc *gpiod;
	int flag;
	int irq;
} ;

static struct gpio_key *gpio_keys;

/* 主设备号                                                                 */
static int major = 0;
static struct class *gpio_key_class;

static int g_key = 0;

static DECLARE_WAIT_QUEUE_HEAD(gpio_key_wait);

/* 实现对应的open/read/write等函数，填入file_operations结构体                   */
static ssize_t gpio_key_drv_read (struct file *file, char __user *buf, size_t size, loff_t *offset)
{
	//printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	int err;
	
	wait_event_interruptible(gpio_key_wait, g_key);
	err = copy_to_user(buf, &g_key, 4);
	g_key = 0;
	
	return 4;
}

/* 定义自己的file_operations结构体                                              */
static struct file_operations gpio_key_drv = {
	.owner	 = THIS_MODULE,
	.read    = gpio_key_drv_read,
};

static irqreturn_t gpio_key_isr(int irq, void *dev_id)
{
	struct gpio_key *gpio_key = dev_id;
	int val;
	val = gpiod_get_value(gpio_key->gpiod);
	
	printk("key %d %d\n", gpio_key->gpio, val);
	g_key = (gpio_key->gpio << 8) | val;
	wake_up_interruptible(&gpio_key_wait);
	
	return IRQ_HANDLED;
}

/* 1. 从platform_device获得GPIO
 * 2. gpio=>irq
 * 3. request_irq
 */
static int gpio_key_probe(struct platform_device *pdev)
{
	int err;
	struct device_node *node = pdev->dev.of_node;
	int count;
	int i;
	enum of_gpio_flags flag;
		
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

	count = of_gpio_count(node);
	if (!count)
	{
		printk("%s %s line %d, there isn't any gpio available\n", __FILE__, __FUNCTION__, __LINE__);
		return -1;
	}

	gpio_keys = kzalloc(sizeof(struct gpio_key) * count, GFP_KERNEL);
	for (i = 0; i < count; i++)
	{
		gpio_keys[i].gpio = of_get_gpio_flags(node, i, &flag);
		if (gpio_keys[i].gpio < 0)
		{
			printk("%s %s line %d, of_get_gpio_flags fail\n", __FILE__, __FUNCTION__, __LINE__);
			return -1;
		}
		gpio_keys[i].gpiod = gpio_to_desc(gpio_keys[i].gpio);
		gpio_keys[i].flag = flag & OF_GPIO_ACTIVE_LOW;
		gpio_keys[i].irq  = gpio_to_irq(gpio_keys[i].gpio);
	}

	for (i = 0; i < count; i++)
	{
		err = request_irq(gpio_keys[i].irq, gpio_key_isr, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "gpio_key", &gpio_keys[i]);
	}

	/* 注册file_operations 	*/
	major = register_chrdev(0, "gpio_key", &gpio_key_drv);  /* /dev/gpio_key */

	gpio_key_class = class_create(THIS_MODULE, "gpio_key_class");  /* /sys/class/gpio_key_class/ */
	if (IS_ERR(gpio_key_class)) {
		printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
		unregister_chrdev(major, "gpio_key");
		return PTR_ERR(gpio_key_class);
	}

	device_create(gpio_key_class, NULL, MKDEV(major, 0), NULL, "gpio_key"); /* /sys/class/gpio_key_class/gpio_key */
        
    return 0;
    
}

static int gpio_key_remove(struct platform_device *pdev)
{
	//int err;
	struct device_node *node = pdev->dev.of_node;
	int count;
	int i;

	device_destroy(gpio_key_class, MKDEV(major, 0));
	class_destroy(gpio_key_class);
	unregister_chrdev(major, "gpio_key");

	count = of_gpio_count(node);
	for (i = 0; i < count; i++)
	{
		free_irq(gpio_keys[i].irq, &gpio_keys[i]);
	}
	kfree(gpio_keys);
    return 0;
}

static const struct of_device_id keys_of_match[] = {
    { .compatible = "mt2712,gpio_key" },
    { },
};

/* 1. 定义platform_driver */
static struct platform_driver gpio_keys_driver = {
    .probe      = gpio_key_probe,
    .remove     = gpio_key_remove,
    .driver     = {
        .name   = "gpio_key",
		.owner = THIS_MODULE,
        .of_match_table = keys_of_match,
    },
};

/* 2. 在入口函数注册platform_driver */
static int __init gpio_key_init(void)
{
    int err;
    
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	
    err = platform_driver_register(&gpio_keys_driver);
    if (err != 0)
        printk("register driver failed (%d)\n", err);	
	
	return err;
}

/* 3. 有入口函数就应该有出口函数：卸载驱动程序时，就会去调用这个出口函数
 *     卸载platform_driver
 */
static void __exit gpio_key_exit(void)
{
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    platform_driver_unregister(&gpio_keys_driver);
}

/* 7. 其他完善：提供设备信息，自动创建设备节点                                     */

module_init(gpio_key_init);
module_exit(gpio_key_exit);

MODULE_LICENSE("GPL");


