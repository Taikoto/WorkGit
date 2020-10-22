have completed the device and dev of leds

void led_class_create_device(int minor);
void led_class_destroy_device(int minor);
void register_led_operations(struct led_operations *opr);

struct led_operations {
        int (*init) (int which); /* init LED, which-which LED */
        int (*ctl) (int which, char status); /* control LED, which-which LED, status:1-on,0-off */
};

