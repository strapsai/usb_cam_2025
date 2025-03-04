#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libudev.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/usbdevice_fs.h>

int main(int argc, char **argv) {
    struct udev *udev;
    struct udev_enumerate *enumerate;
    struct udev_list_entry *devices, *dev_list_entry;
    struct udev_device *dev;

    if (argc != 4) {
        fprintf(stderr, "Usage: usbreset <vendor_id> <product_id> <serial_number>\n");
        return 1;
    }

    const char *vendor_id = argv[1];
    const char *product_id = argv[2];
    const char *serial_number = argv[3];

    udev = udev_new();
    if (!udev) {
        fprintf(stderr, "Can't create udev\n");
        return 1;
    }

    enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "usb");
    udev_enumerate_scan_devices(enumerate);
    devices = udev_enumerate_get_list_entry(enumerate);

    udev_list_entry_foreach(dev_list_entry, devices) {
        const char *path = udev_list_entry_get_name(dev_list_entry);
        dev = udev_device_new_from_syspath(udev, path);

        const char *dev_vendor_id = udev_device_get_sysattr_value(dev, "idVendor");
        const char *dev_product_id = udev_device_get_sysattr_value(dev, "idProduct");
        const char *dev_serial = udev_device_get_sysattr_value(dev, "serial");

        if (dev_vendor_id && dev_product_id && dev_serial &&
            strcmp(dev_vendor_id, vendor_id) == 0 &&
            strcmp(dev_product_id, product_id) == 0 &&
            strcmp(dev_serial, serial_number) == 0) {
            
            const char *devnode = udev_device_get_devnode(dev);
            if (devnode) {
                int fd = open(devnode, O_WRONLY);
                if (fd < 0) {
                    perror("Error opening device");
                    udev_device_unref(dev);
                    continue;
                }

                printf("Resetting USB device %s\n", devnode);
                if (ioctl(fd, USBDEVFS_RESET, 0) < 0) {
                    perror("Error in ioctl");
                    close(fd);
                    udev_device_unref(dev);
                    continue;
                }

                printf("Reset successful\n");
                close(fd);
                udev_device_unref(dev);
                udev_enumerate_unref(enumerate);
                udev_unref(udev);
                return 0;
            }
        }
        udev_device_unref(dev);
    }

    fprintf(stderr, "Device not found\n");
    udev_enumerate_unref(enumerate);
    udev_unref(udev);
    return 1;
}
