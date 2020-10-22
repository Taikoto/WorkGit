struct led_operations {
        int (*init) (int which); /* init LED, which-which LED */
        int (*ctl) (int which, char status); /* control LED, which-which LED, status:1-on,0-off */
};

It is compiled with cake