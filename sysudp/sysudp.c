// sysudp.c
// make -C /lib/modules/`uname -r`/build SUBDIRS=`pwd` modules
// insmod ./sysudp.ko
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sysfs.h>
#include <linux/ip.h>
#include <linux/in.h>

static struct kobject *udp_kobject, *srv;
static char ctrl, cctrl, data;

struct socket *ksock;
struct sockaddr_in addr, raddr;
struct msghdr msg;
struct iovec iov;
mm_segment_t oldfs;

static int bind_socket(unsigned short port)
{
	memset(&addr, 0, sizeof(struct sockaddr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);

	if (ksock->ops->bind(ksock, (struct sockaddr*)&addr, sizeof(struct sockaddr)) < 0)
		return -1;

	return 0;
}

static ssize_t cctrl_store(struct kobject *kobj, struct kobj_attribute *attr, char *buf, size_t count)
{
	// 为了代码简短，未做字符串解析，采用了硬编码，且仅支持一个socket的创建
	if (!strcmp(buf, "connect 127.0.0.1:321")) { // 写ctrl文件实现connect
		memset(&raddr, 0, sizeof(struct sockaddr));
		raddr.sin_family = AF_INET;
		raddr.sin_addr.s_addr = htonl(0x7f000001);
		raddr.sin_port = htons(321);
	} else if (strstr(buf, "bind 127.0.0.1:123")) { // 写ctrl文件实现bind
		bind_socket(123);
	} else if (strstr(buf, "setsockopt")) { // sockopt也是写文件完成
		// TODO
	}
	return count;
}

static ssize_t cctrl_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "bind x.x.x.x:yyy\nconnect x.x.x.x:yyy\nsetsockopt value\n[TODO]....\n");
}

static ssize_t data_store(struct kobject *kobj, struct kobj_attribute *attr, char *buf, size_t count)
{
	int size = 0;

	if (ksock->sk == NULL) return 0;

	iov.iov_base = buf;
	iov.iov_len = count;
	msg.msg_flags = 0;
	msg.msg_name = &raddr;
	msg.msg_namelen  = sizeof(struct sockaddr_in);
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = NULL;

	oldfs = get_fs();
	set_fs(KERNEL_DS);
	size = sock_sendmsg(ksock, &msg, count);
	set_fs(oldfs);

	return size;
}

static ssize_t data_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	int size = 2048;

	if (ksock->sk == NULL) return 0;

	iov.iov_base = buf;
	iov.iov_len = size;
	msg.msg_flags = 0;
	msg.msg_name = &addr;
	msg.msg_namelen  = sizeof(struct sockaddr_in);
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = NULL;

	oldfs = get_fs();
	set_fs(KERNEL_DS);
	size = sock_recvmsg(ksock, &msg, size, msg.msg_flags);
	set_fs(oldfs);

	return size;
}

static struct kobj_attribute ctrl_attribute =__ATTR(ctrl, 0660, cctrl_show, cctrl_store);
static struct kobj_attribute data_attribute =__ATTR(data, 0660, data_show, data_store);

static ssize_t ctrl_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "create\nshutdown\n[TODO]....\n");
}

static int create_socket()
{
	if (sock_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &ksock) < 0) 
		return -1;

	return 0;
}

static ssize_t ctrl_store(struct kobject *kobj, struct kobj_attribute *attr, char *buf, size_t count)
{
	// 仅支持一个socket
	if (!strcmp(buf, "create")) { // 写ctrl文件创建socket实例
		if (!srv) {
			srv = kobject_create_and_add("instance_0", udp_kobject);
        	sysfs_create_file(srv, &ctrl_attribute.attr);
        	sysfs_create_file(srv, &data_attribute.attr);
			create_socket();
		}
	} else if (!strcmp(buf, "shutdown")) { // 写ctrl文件销毁socket实例
		if (srv)
			if (ksock)
				sock_release(ksock);
			kobject_put(srv);
			srv = NULL;
	}

	return count;
}
static struct kobj_attribute foo_attribute =__ATTR(ctrl, 0660, ctrl_show, ctrl_store);

static int __init sysudp_init (void)
{
	int error = 0;

	srv = NULL;
	udp_kobject = kobject_create_and_add("kobject_udp", NULL);
	if(!udp_kobject)
		return -ENOMEM;

	error = sysfs_create_file(udp_kobject, &foo_attribute.attr);

	return error;
}

static void __exit sysudp_exit (void)
{
	if (srv) {
		if (ksock)
			sock_release(ksock);
		kobject_put(srv);
	}
	kobject_put(udp_kobject);
}
MODULE_LICENSE("GPL");
module_init(sysudp_init);
module_exit(sysudp_exit);

