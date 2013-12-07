/*
 * tcs3200_dev.c
 *
 *  Created on: Dec 5, 2013
 *      Author: flo
 */

#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include "common.h"

static int tcs_open(struct inode *inode, struct file *f) {
	struct tcs_dev *tcs;

	tcs = container_of(inode->i_cdev, struct tcs_dev, cdev);
	f->private_data = tcs; /* for later use */
	return 0;
}

static int tcs_close(struct inode *i, struct file *f) {
	struct tcs_dev *tcs = f->private_data;

	tcs_stop_measurement(tcs); //just to be sure
	return 0;
}

static ssize_t tcs_read(struct file *f, char __user *buf, size_t count, loff_t *off) {
	DECLARE_WAITQUEUE(wait, current);
	struct tcs_dev *tcs = f->private_data;
	size_t max = sizeof(struct tcs3200_measurement);

	printk(KERN_ERR "%s:%s:reading %d bytes with offset %d\n", KBUILD_MODNAME, __FUNCTION__, count, (int)*off);
	if (*off >= max || !count)
		return 0;

	tcs_start_measurement(tcs);
	add_wait_queue(&tcs->waitq, &wait);
	set_current_state(TASK_INTERRUPTIBLE);
	schedule();
	if(signal_pending(current)) {
		set_current_state(TASK_RUNNING);
		remove_wait_queue(&tcs->waitq, &wait);
		return -ERESTARTSYS;
	}
	remove_wait_queue(&tcs->waitq, &wait);

	if(*off + count > max)
		count = max - *off;

	if(copy_to_user(buf, &tcs->measurement + *off, count))
		return -EFAULT;

	*off += count;
	printk(KERN_INFO "read %d bytes\n", count);
	return count;
}

static ssize_t tcs_write(struct file *f, const char __user *buf, size_t len, loff_t *off) {

	return -EINVAL;
}

static struct file_operations tcs_fops = {
		.owner = THIS_MODULE,
		.llseek = no_llseek,
		.open = tcs_open,
		.release = tcs_close,
		.read = tcs_read,
		.write = tcs_write
};

static struct tcs_dev *tcs;

static int __init tcs3200_init(void) {
	struct device *dev;
	int rc;

	tcs = kzalloc(sizeof(struct tcs_dev), GFP_KERNEL);
	if(tcs == NULL) {
		printk(KERN_ERR "%s:%s:%s\n", KBUILD_MODNAME, __FUNCTION__, "malloc failed");
		rc = -ENOMEM;
		goto mem_fail;
	}
	rc = alloc_chrdev_region(&tcs->devid, 0, 1, "tcs3200");
	if(rc < 0) {
		printk(KERN_ERR "%s:%s:%s\n", KBUILD_MODNAME, __FUNCTION__, "failed to alloc cdev");
		goto cdev_fail;
	}
	cdev_init(&tcs->cdev, &tcs_fops);
	rc = cdev_add(&tcs->cdev, tcs->devid, 1);
	if(rc < 0) {
		printk(KERN_ERR "%s:%s:%s\n", KBUILD_MODNAME, __FUNCTION__, "failed to add cdev");
		goto add_fail;
	}
	tcs->class = class_create(THIS_MODULE, "tcs3200");
	if(IS_ERR(tcs->class)) {
		rc = PTR_ERR(tcs->class);
		goto class_fail;
	}
	dev = device_create(tcs->class, NULL, tcs->devid, NULL, "tcs0");
	if(IS_ERR(dev)) {
		rc = PTR_ERR(dev);
		goto dev_fail;
	}
	init_waitqueue_head(&tcs->waitq);
	tcs_control_init(tcs);
	tcs_counter_init(tcs);
	printk(KERN_INFO "tcs3200 loaded\n");
	return 0;

dev_fail:
	class_destroy(tcs->class);
class_fail:
	cdev_del(&tcs->cdev);
add_fail:
	unregister_chrdev_region(tcs->devid, 1);
cdev_fail:
	kfree(tcs);
mem_fail:
	return rc;
}

static void __exit tcs3200_free(void) {

	tcs_counter_exit(tcs);
	tcs_control_exit();
	device_destroy(tcs->class, tcs->devid);
	class_destroy(tcs->class);
	cdev_del(&tcs->cdev);
	unregister_chrdev_region(tcs->devid, 1);
	kfree(tcs);
	printk(KERN_INFO "tcs3200 removed\n");
}

module_init(tcs3200_init);
module_exit(tcs3200_free);

MODULE_AUTHOR("Florian Hercher");
MODULE_DESCRIPTION("TCS3200 driver");
MODULE_LICENSE("GPL");

