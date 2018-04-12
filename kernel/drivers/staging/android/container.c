/* container.c
 *
 * container manager driver
 *
 * Copyright (C) 2016-2020 cells, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <asm/cacheflush.h>
#include <linux/fdtable.h>
#include <linux/file.h>
#include <linux/freezer.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/nsproxy.h>
#include <linux/poll.h>
#include <linux/debugfs.h>
#include <linux/rbtree.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/pid_namespace.h>
#include <linux/security.h>
#include <linux/proc_fs.h>
#include <linux/drv_namespace.h>
#include <linux/rwsem.h>
#include <linux/container.h>

static struct drv_namespace *idx2ns[MAX_CONTAINER];
static int idx2pos[MAX_CONTAINER];

static DEFINE_MUTEX(ct_lock);
static wait_queue_head_t pos_wait;

static int ril_request_token;

static inline void container_lock(void)
{
	mutex_lock(&ct_lock);
}

static inline void container_unlock(void)
{
	mutex_unlock(&ct_lock);
}


void container_add(struct drv_namespace *drv_ns)
{
	int i, j;
	int is_registered = false;
	int is_changed = false;

	container_lock();
	for (i = 0; i < MAX_CONTAINER; i++) {
		if (drv_ns == idx2ns[i]) {
			pr_info("[container_add] the drv_ns already registered!\n");
			is_registered = true;
		}
	}

	if (!is_registered) {
		for (i = 0; i < MAX_CONTAINER; i++) {
			if (0 == idx2ns[i]) {
				idx2ns[i] = drv_ns;
				/* sort will decresee 1 */
				idx2pos[i] = MAX_CONTAINER + 1;
				is_changed = true;
				pr_info("[container_add] register drv_ns[%p] at container [%d]!\n",
				       drv_ns, i);
				break;
			}
		}

		if (!is_changed)
			pr_info("[container_add] too many container created! max[%d]\n",
			       MAX_CONTAINER);
		else {
			/* register success! need resort pos
			   the pos is sorted to MAX_CONTAINER... 1,
			   the new container will awalys at MAX_CONTAINER
			   to make sure in foreground */
			for (j = 0; j < MAX_CONTAINER; j++) {
				if (0 != idx2pos[j]) {
					idx2pos[j]--;
					pr_info("[container_add] set container[%d] pos[%d]!\n",
					       j, idx2pos[j]);
				}
			}
		}
	}

	container_unlock();

	if (is_changed)
		wake_up(&pos_wait);
}

void container_switch(struct drv_namespace *drv_ns)
{
	int i;
	int idx;

	container_lock();

	/* get container idx by drv_ns */
	for (i = 0; i < MAX_CONTAINER; i++) {
		if (drv_ns == idx2ns[i]) {
			idx = i;
			break;
		}
	}

	if (i >= MAX_CONTAINER)
		pr_err("[container_switch] the drv_ns not registered!\n");
	else {
		/* find foreground container */
		for (i = 0; i < MAX_CONTAINER; i++) {
			if (MAX_CONTAINER == idx2pos[i])
				break;
		}

		/* switch pos */
		idx2pos[i] = idx2pos[idx];
		idx2pos[idx] = MAX_CONTAINER;
	}

	container_unlock();
	wake_up(&pos_wait);
}

static long container_ioctl(struct file *filp, unsigned int cmd,
			    unsigned long arg)
{
	int ret;
	unsigned int size = _IOC_SIZE(cmd);
	void __user *ubuf = (void __user *)arg;
	int idx = (0xFFFFFFFF & (long)filp->private_data);
	struct container_info_t info;
	struct drv_namespace *drv_ns = current_drv_ns();

	switch (cmd) {
	case CONTAINER_GET_IDX: {
		if (copy_to_user(ubuf, &idx, sizeof(idx))) {
			ret = -EFAULT;
			goto err;
		}
		break;
	}
	case CONTAINER_GET_POS: {
		int pos;
		sleep_on(&pos_wait);
		container_lock();
		pos = idx2pos[idx];
		container_unlock();
		if (copy_to_user(ubuf, &pos, sizeof(pos))) {
			ret = -EFAULT;
			goto err;
		}
		break;
	}
	case CONTAINER_GET_INFO: {
		if (size != sizeof(info)) {
			ret = -EINVAL;
			goto err;
		}

		sleep_on(&pos_wait);

		container_lock();
		info.idx = idx;
		info.pos = idx2pos[idx];
		info.drv_ns = (void *)idx2ns[idx];
		container_unlock();
		if (copy_to_user(ubuf, &info, sizeof(info))) {
			ret = -EFAULT;
			goto err;
		}
		break;
	}
	case CONTAINER_SET_INFO: {
		if (size != sizeof(info)) {
			ret = -EINVAL;
			goto err;
		}

		if (copy_from_user(&info, ubuf, sizeof(info))) {
			ret = -EFAULT;
			goto err;
		}

		if ((info.idx != idx)
		    || (info.drv_ns != (void *)idx2ns[idx])) {
			pr_err("[container_ioctl] error info, idx[%d-%d] drv_ns[%p-%p]!\n",
				info.idx, idx, info.drv_ns, idx2ns[idx]);
			ret = -EINVAL;
			goto err;
		}

		container_lock();
		idx2pos[idx] = info.pos;
		container_unlock();
		break;
	}
	case CONTAINER_GET_INT: {
		container_lock();
		ril_request_token++;
		if (ril_request_token < 0)
			ril_request_token = 0;
		if (copy_to_user(ubuf, &ril_request_token, sizeof(ril_request_token))) {
			container_unlock();
			ret = -EFAULT;
			goto err;
		}
		container_unlock();
		break;
	}
	case CONTAINER_GET_MODE:
	case CONTAINER_GET_ACTIVE_MODE: {
		int mode_status = 0;
		int active_status = 0;
		int mode_rst;

		if (is_init_ns(drv_ns))
			mode_status = 100;
		else if (!strcmp(drv_ns->tag, SECURITY_SYSTEM_TAG))
			mode_status = 1;

		if (drv_ns->active)
			active_status = 1;

		if (cmd == CONTAINER_GET_MODE) {
			mode_rst = mode_status;
		} else {
			mode_rst = active_status;
		}
		if (copy_to_user(ubuf, &mode_rst, sizeof(int))) {
			ret = -EFAULT;
			goto err;
		}
		break;
	}
	default:
		ret = -EINVAL;
		goto err;
	}
	ret = 0;
err:
	return ret;
}

static int container_open(struct inode *nodp, struct file *filp)
{
	struct drv_namespace *drv_ns = current_drv_ns();
	int i;
	int idx = -1;

	container_lock();
	for (i = 0; i < MAX_CONTAINER; i++) {
		if (drv_ns == idx2ns[i])
			idx = i;
	}
	container_unlock();

	filp->private_data = (void *)((long)idx);
	return 0;
}

static int container_release(struct inode *nodp, struct file *filp)
{
	return 0;
}

static const struct file_operations container_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = container_ioctl,
	.compat_ioctl = container_ioctl,
	.open = container_open,
	.release = container_release,
};

static struct miscdevice container_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "container",
	.fops = &container_fops
};

/* proc */
/* proc entries */
static struct proc_dir_entry *container_proc_root;
static struct proc_dir_entry *container_proc_info;

static int container_proc_show(struct seq_file *s, void *p)
{
	int i;

	container_lock();
	for (i = 0; i < MAX_CONTAINER; i++) {
		seq_printf(s, "container[%d] pos[%d] drv_ns[%p]\n", i,
			   idx2pos[i], idx2ns[i]);
	}
	container_unlock();
	return 0;
}

static int container_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, container_proc_show, NULL);
}

static const struct file_operations container_proc_fops = {
	.open = container_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int __init container_init(void)
{
	int ret;

	init_waitqueue_head(&pos_wait);
	ret = misc_register(&container_miscdev);
	if (ret)
		return ret;

	container_proc_root = proc_mkdir("container", NULL);
	if (container_proc_root == NULL) {
		ret = -ENOMEM;
		pr_err("container error: cannot create proc dir /proc/container\n");
	} else {
		container_proc_info = proc_create("info", 0666,
			container_proc_root, &container_proc_fops);
		if (container_proc_info == NULL) {
			pr_err("container error: cannot create proc entry /proc/container/info\n");
			ret = -ENOMEM;
		}
	}
	return ret;
}

device_initcall(container_init);

MODULE_LICENSE("GPL v2");
