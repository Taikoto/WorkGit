#include <sys/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
 
#define UEVENT_MSG_LEN 4096
 
int device_fd = -1;
 
struct listener_gliethttp {
    const char *action;
    const char *path;
    const char *subsystem;
    const char *firmware;
    int major;
    int minor;
};

static int init_socket(void)
{
    struct sockaddr_nl addr;
    int sz = 64*1024;
    int s;
 
    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = getpid();
    addr.nl_groups = 0xffffffff;
 
    s = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);// uevent_fd_.Set(socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT));
    if (s < 0) {
        printf("socket failed \r\n");
        return -1;
    }
 
    setsockopt(s, SOL_SOCKET, SO_RCVBUFFORCE, &sz, sizeof(sz));
 
    if (bind(s, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        printf("bind failed\r\n");
        close(s);
        return -1;
    }
 
    return s;
}

static void parse_event(const char *msg, struct listener_gliethttp *listener_gliethttp)
{
    listener_gliethttp->action = "";
    listener_gliethttp->path = "";
    listener_gliethttp->subsystem = "";
    listener_gliethttp->firmware = "";
    listener_gliethttp->major = -1;
    listener_gliethttp->minor = -1;

    printf("========================================================\n");
    while (*msg) { 
        printf("%s\n", msg);
 
        if (!strncmp(msg, "ACTION=", 7)) {
            msg += 7;
            listener_gliethttp->action = msg;
        } else if (!strncmp(msg, "DEVPATH=", 8)) {
            msg += 8;
            listener_gliethttp->path = msg;
        } else if (!strncmp(msg, "SUBSYSTEM=", 10)) {
            msg += 10;
            listener_gliethttp->subsystem = msg;
        } else if (!strncmp(msg, "FIRMWARE=", 9)) {
            msg += 9;
            listener_gliethttp->firmware = msg;
        } else if (!strncmp(msg, "MAJOR=", 6)) {
         msg += 6;
            listener_gliethttp->minor = atoi(msg);
        } else if (!strncmp(msg, "PANEL_ALIVE=", 12)) {
            printf("PANEL_ALIVE==%d\n",atoi(msg));
        }
 
        while(*msg++)
            ;
    }

    printf("event { '%s', '%s', '%s', '%s', %d, %d }\n",
            listener_gliethttp->action, listener_gliethttp->path, listener_gliethttp->subsystem,
            listener_gliethttp->firmware, listener_gliethttp->major, listener_gliethttp->minor);
}

void UEventHandler(void)
{
    char buffer[1024];
    int ret;
    char *event;

    while (1) {
        printf("*******************************************************************\n");
	printf("waiting data sent for kernel!\n");  
        ret = read(device_fd, &buffer, sizeof(buffer));
        if (ret == 0) {
            return;
        } else if (ret < 0) {
            printf("Got error reading uevent %d", ret);
            return;
        }

	printf("recevied message:\r\n");
        for (int i = 0; i < ret;) {
            event = buffer + i;
            if (strcmp(event, "DEVTYPE=drm_minor"))
                ;
            else if (strcmp(event, "HOTPLUG=1")) {
                printf("hwc_uevent detect hotplug");
            }
    
            printf("%s \r\n",event);
            i += strlen(event) + 1;
        }
    }
}
 
int main(void)
{
    char msg[UEVENT_MSG_LEN+2];
    int n;
    struct listener_gliethttp listener_gliethttp;

    device_fd = init_socket();
    if(device_fd < 0){
        printf("fail to init socket!\r\n");
        return 0;
    } else {
        printf("success to init socket!\r\n");
    }
		
    //UEventHandler();
    while(1) {
        while((n = recv(device_fd, msg, UEVENT_MSG_LEN, 0)) > 0) {
            printf("recv msg\r\n");
            if(n == UEVENT_MSG_LEN)
                continue;
 
            msg[n] = '\0';
            msg[n+1] = '\0';
            
            printf("-----------   msg=%s\n",msg);
            parse_event(msg, &listener_gliethttp);
            printf("-----------   msg=%s\n",msg);
        }
    }
}
