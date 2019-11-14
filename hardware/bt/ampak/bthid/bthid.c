/*
*
* bthid.c
*
*
*
* Copyright (C) 2015 Broadcom Corporation.
*
*
*
* This software is licensed under the terms of the GNU General Public License,
* version 2, as published by the Free Software Foundation (the "GPL"), and may
* be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
* or FITNESS FOR A PARTICULAR PURPOSE. See the GPL for more details.
*
*
* A copy of the GPL is available at http://www.broadcom.com/licenses/GPLv2.php
* or by writing to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
* Boston, MA 02111-1307, USA
*
*
*/

#include <linux/version.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>

#include <linux/usb.h>

#include <linux/hid.h>
#include <linux/hiddev.h>
#include <linux/hid-debug.h>
#include "bthid.h"

struct bthid_device
{
    /* reference to the created HID device */
    struct hid_device *hdev;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)
    /* contains the current descriptor for this HID device */
    BTHID_CONTROL hid_desc;
#endif
    BTHID_CONFIG config;
};

/* Declaration of brcm_linux.c functions */
static int bthid_open(struct inode *inode, struct file *filp);
static int bthid_release(struct inode *inode, struct file *filp);
static ssize_t bthid_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
static long bthid_ioctl(struct file *file,
        unsigned int cmd, unsigned long arg);
#else
static int bthid_ioctl(struct inode *inode, struct file *file,
        unsigned int cmd, unsigned long arg);
#endif

static void bthid_exit(void);
static int bthid_init(void);
static void _bt_detach_from_hid(struct bthid_device *bthdev);

static int bthid_ll_open(struct hid_device *hdev)
{
    BTHID_DBG("\n");
    return 0;
}

static void bthid_ll_close(struct hid_device *hdev)
{
    BTHID_DBG("\n");
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,15,0)
static int bthid_ll_hidinput_event(struct input_dev *idev, unsigned int type,
                                   unsigned int code, int  value)
{
    BTHID_DBG("Keycode = %x\n",value);
    return 0;
}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,15,0)
static int bthid_ll_raw_request(struct hid_device *hdev, unsigned char reportnum,
                                __u8 *buf, size_t len, unsigned char rtype,
                                int reqtype)
{
    BTHID_DBG("reportnum = %x\n",reportnum);
    return 0;
}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)
static int bthid_ll_parse(struct hid_device *hdev)
{
    /* retrieve the HID descriptor reference */
    struct bthid_device *bthdev = hdev->driver_data;
    int ret;

    BTHID_DBG("\n");
    ret = hid_parse_report(hdev, bthdev->hid_desc.data, bthdev->hid_desc.size);

    BTHID_DBG("hid_parse_report ret %d\n", ret);

    return ret;
}

static int bthid_ll_start(struct hid_device *hdev)
{
    BTHID_DBG("\n");
    return 0;
}

static void bthid_ll_stop(struct hid_device *hdev)
{
    BTHID_DBG("\n");
}

static struct hid_ll_driver bthid_hid_driver =
{
    .parse = bthid_ll_parse,
    .start = bthid_ll_start,
    .stop = bthid_ll_stop,
    .open  = bthid_ll_open,
    .close = bthid_ll_close,
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,15,0)
    .hidinput_input_event = bthid_ll_hidinput_event
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,15,0)
    .raw_request = bthid_ll_raw_request
#endif
};
#endif

static int _bt_attach_to_hid(struct bthid_device *bthdev,
        BTHID_CONTROL *p_btctrl)
{
    struct hid_device *hdev;

    BTHID_DBG("rd_data %p rd_size %d\n", p_btctrl->data,
            p_btctrl->size);

    if (p_btctrl->size > 0)
    {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)
        /* copy the HID descriptor in the BTHID driver structure */
        bthdev->hid_desc = *p_btctrl;

        hdev = hid_allocate_device();
        if (!hdev)
        {
            BTHID_ERR("hid allocation device failed\n");
            return -ENOMEM;
        }
#else
        hdev = hid_parse_report(p_btctrl->data, p_btctrl->size);

        if (!hdev)
        {
            BTHID_ERR("hid_parse_report failed\n");
            return -EINVAL;
        }
#endif
        /* save the HID structure reference */
        bthdev->hdev = hdev;

        /* initialize the HID structure */
        hdev->bus = BUS_BLUETOOTH;
        hdev->country = bthdev->config.country;
        hdev->vendor = bthdev->config.vid;
        hdev->product = bthdev->config.pid;
        hdev->version = bthdev->config.version;
        strncpy(hdev->name, bthdev->config.name, sizeof(hdev->name));
        hdev->name[sizeof(hdev->name)-1] = 0;

        /* save the BTHID driver reference in the HID device structure */
        hdev->driver_data = bthdev;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)
        hdev->ll_driver = &bthid_hid_driver;

        /* add the HID device */
        if (hid_add_device(hdev))
        {
            BTHID_ERR("hid_add_device failed\n");
            hid_destroy_device(hdev);
            bthdev->hdev = NULL;
            return -ENOMEM;
        }

#else
        hdev->dev = NULL;

        hdev->hid_open = bthid_ll_open;
        hdev->hid_close = bthid_ll_close;
        hdev->hidinput_input_event = bthid_ll_hidinput_event;

        if (hidinput_connect(hdev) == 0)
        {
            BTHID_DBG("HID Interface Claimed...\n");
            hdev->claimed |= HID_CLAIMED_INPUT;
        }
        else
        {
            BTHID_ERR("hidinput_connect failed\n");
        }
#endif
    }
    else
    {
        BTHID_ERR("No descriptor specified\n");
        return -EINVAL;
    }
    return 0;

}

static void _bt_detach_from_hid(struct bthid_device *bthdev)
{
    BTHID_DBG("++\n");

    if (bthdev->hdev)
    {
        // if (bthdev->hdev->claimed & HID_CLAIMED_INPUT)
        BTHID_DBG("Free HID input device\n");
        hidinput_disconnect(bthdev->hdev);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)
        hid_destroy_device(bthdev->hdev);
#else
        hid_free_device(bthdev->hdev);
#endif
        bthdev->hdev = NULL;
    }

    BTHID_DBG("--\n");
}

static int bthid_open(struct inode *inode, struct file *file)
{
    struct bthid_device *bthdev;

    BTHID_DBG("++\n");

    /* allocate a new driver structure */
    bthdev = kzalloc(sizeof(*bthdev), GFP_KERNEL);
    if (!bthdev)
    {
        return -ENOMEM;
    }

    strncpy(bthdev->config.name, "Broadcom Bluetooth HID", sizeof(bthdev->config.name));
    bthdev->config.name[sizeof(bthdev->config.name)-1] = 0;

    /* save the driver structure reference in the file structure */
    file->private_data = bthdev;

    BTHID_DBG("--\n");
    return 0;
}

static int bthid_release(struct inode *inode, struct file *file)
{
    /* retrieve the driver structure reference from the file structure */
    struct bthid_device *bthdev = file->private_data;

    BTHID_DBG("++\n");
    /* check that the driver was correctly allocated */
    if (bthdev)
    {
        _bt_detach_from_hid(bthdev);
        kfree(bthdev);
    }

    BTHID_DBG("--\n");
    return 0;
}

static ssize_t bthid_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int retval;
    /* retrieve the driver structure reference from the file structure */
    struct bthid_device *bthdev = file->private_data;
    unsigned char *buf = kmalloc(count + 1, GFP_KERNEL);

    if (!buf)
    {
        return -ENOMEM;
    }

    if (copy_from_user(buf, buffer, count))
    {
        kfree(buf);
        return -EFAULT;
    }

    if (bthdev->hdev)
    {
        BTHID_INFO("sending report to kernel len:%d\n", count);
        retval = hid_input_report(bthdev->hdev, HID_INPUT_REPORT, buf, count, 1);
    }
    else
    {
        BTHID_ERR("Writing while descriptor not provided\n");
        retval = -EINVAL;
    }

    kfree(buf);

    return retval;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
static long bthid_ioctl(struct file *file,
        unsigned int cmd, unsigned long arg)
#else
static int bthid_ioctl(struct inode *inode, struct file *file,
        unsigned int cmd, unsigned long arg)
#endif
{
    int retval = 0;
    struct bthid_device *bthdev;
    BTHID_CONTROL *p_btctrl;

    BTHID_DBG("cmd=%u\n", cmd);

    /* retrieve the driver private pointer */
    bthdev = file->private_data;

    /* check that the driver was correctly opened */
    if (bthdev == NULL)
    {
        BTHID_ERR("no device\n");
        return -ENODEV;
    }

    switch (cmd)
    {
    case BTHID_PARSE_HID_DESC:
        BTHID_DBG("BTHID_PARSE_HID_DESC\n");
        /* allocate space in kernel space to copy the user data */
        p_btctrl = kmalloc(sizeof(*p_btctrl), GFP_KERNEL);
        if (!p_btctrl)
        {
            return -ENOMEM;
        }

        /* copy the user data in kernel space */
        if (copy_from_user(p_btctrl, (void __user *)arg, sizeof(*p_btctrl)))
        {
            BTHID_ERR("copy_from_user failed\n");
            kfree(p_btctrl);
            return -EFAULT;
        }

        retval = _bt_attach_to_hid(bthdev, p_btctrl);
        kfree(p_btctrl);
        break;

    case BTHID_SET_CONFIG:
        BTHID_DBG("BTHID_SET_CONFIG\n");
        if (!arg)
        {
            return -EFAULT;
        }
        /* copy the user data in kernel space */
        if (copy_from_user(&bthdev->config, (void __user *)arg, sizeof(bthdev->config)))
        {
            BTHID_ERR("copy_from_user failed\n");
            return -EFAULT;
        }
        break;

    case BTHID_GET_CONFIG:
        BTHID_DBG("BTHID_GET_CONFIG\n");
        if (!arg)
        {
            return -EFAULT;
        }
        /* copy the user data in kernel space */
        if (copy_to_user((void __user *)arg, &bthdev->config, sizeof(bthdev->config)))
        {
            BTHID_ERR("copy_to_user failed\n");
            return -EFAULT;
        }
        break;

    default:
        BTHID_ERR("unknown command\n");
        retval = -EINVAL;
        break;
    }

    BTHID_DBG("-\n");
    return retval;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)
static const struct hid_device_id bthid_table[] =
{
    { HID_BLUETOOTH_DEVICE(HID_ANY_ID, HID_ANY_ID) },
    { }
};

static struct hid_driver bthid_driver =
{
    .name = BTHID_NAME,
    .id_table = bthid_table,
};
#endif

static const struct file_operations bthid_fops =
{
    .owner = THIS_MODULE,
    .open = bthid_open,
    .release = bthid_release,
    .write = bthid_write,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
    .unlocked_ioctl = bthid_ioctl
#else
    .ioctl = bthid_ioctl
#endif
};

static struct miscdevice bthid_misc =
{
    .fops = &bthid_fops,
    .minor = BTHID_MINOR,
    .name = BTHID_NAME
};

static int __init bthid_init(void)
{
    int ret;
    BTHID_DBG("++\n");

    ret = misc_register(&bthid_misc);

    if (ret)
    {
        BTHID_ERR("misc driver registration error\n");
    }
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)
    ret = hid_register_driver(&bthid_driver);
    if (ret)
    {
        BTHID_ERR("hid driver registration error\n");
    }
#endif
    BTHID_DBG("--\n");

    return ret;
}

static void __exit bthid_exit(void)
{
    BTHID_DBG("++\n");
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)
    hid_unregister_driver(&bthid_driver);
#endif
    misc_deregister(&bthid_misc);
    BTHID_DBG("--\n");
}

module_init(bthid_init);
module_exit(bthid_exit);

MODULE_AUTHOR("Broadcom Corporation");
MODULE_DESCRIPTION("User level driver support for Bluetooth Hid input");
MODULE_LICENSE("GPL");

