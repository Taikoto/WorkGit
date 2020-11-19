#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>
#include <linux/syscalls.h>
#include <asm/unistd.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/i2c.h>
#include <linux/input.h>
 
struct device *dev = NULL;
char *mesg[2];

static ssize_t send_message(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{
    int ret = 0;
    char s_buf[100] = {0};
    char *event_string = "HOTPLUG=1";

    mesg[0] = s_buf;
    //mesg[0]= "HOTPLUG=1";
    mesg[1] = NULL;
    printk("%s\n",event_string);	
    //strcat(s_buf, "kobject_uevent_env hello");
    strcat(s_buf, buf);
    printk(KERN_WARNING "received message buf=%s s_buf=%s count=%zu\n",buf,s_buf,count);
    ret = kobject_uevent_env(&dev->kobj, KOBJ_CHANGE, mesg);
    if(ret < 0) {
        printk("kobject_uevent_env failed, ret = %d\r\n",ret);
    }
    
    return count;
}

static DEVICE_ATTR(m_point, S_IRUGO|S_IWUSR, NULL, send_message);
 
static const struct attribute *kobject_event_attr[] = {
    &dev_attr_m_point.attr,
    NULL,
};
 
static const struct attribute_group kobject_event_attr_group = {
    .attrs = (struct attribute **) kobject_event_attr,
};
 
static struct class kobject_event_class = {
    .name  = "kobject_event",
    .owner = THIS_MODULE,
};
 
static int __init kobject_uevent_init(void)
{
    int ret = 0;

    ret = class_register(&kobject_event_class);

    if (ret < 0) {
        printk(KERN_ERR "song_event: class_register fail\n");
        return ret;
    }
 
    dev = device_create(&kobject_event_class, NULL, MKDEV(0, 0), NULL, "kobject_event");
    if (dev) {
        ret = sysfs_create_group(&dev->kobj, &kobject_event_attr_group);
        if(ret < 0) {
            printk(KERN_ERR "kobject_event:sysfs_create_group fail\r\n");
            return ret;
        }

    } else {
        printk(KERN_ERR "kobject_event:device_create fail\r\n");
        ret = -1;
        return ret;
    }

    return 0;
}

static __exit void kobject_uevent_exit(void)
{
    device_destroy(&kobject_event_class, MKDEV(0,0));
    class_destroy(&kobject_event_class);
    sysfs_remove_group(&dev->kobj, &kobject_event_attr_group);
    printk(KERN_WARNING "kobject_uevent_exit!\n");
}
 
module_init(kobject_uevent_init);
module_exit(kobject_uevent_exit);
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Taikoto Cai");
