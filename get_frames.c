#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/syscalls.h>
#include <linux/vmalloc.h>
#include <linux/kthread.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
#define GET_FRAME_BUFFER _IOWR('a', 2, int) 

struct yuv_frame {
	int frame_no;
	u8 *data;
} *p, *vbuf;

struct yuv_data {
	int framerate;
	int framecount;
	int width;
	int height;
} frame_data;

dev_t device;
struct cdev *cdev;
char name[] = "frame_feed";
struct class *class;
bool I_FLAG;

void* get_frame(void)
{
	return vbuf;
}
EXPORT_SYMBOL(get_frame);

bool get_flag(void)
{
	return I_FLAG;
}
EXPORT_SYMBOL(get_flag);

void set_flag(bool value)
{
	I_FLAG = value;
}
EXPORT_SYMBOL(set_flag);

struct yuv_data get_frame_data(void)
{
	return frame_data;
}
EXPORT_SYMBOL(get_frame_data);

int frame_driver_open(struct inode *inode, struct file *filp)
{
	return 0;
}

int frame_driver_release(struct inode *inode, struct file *filp)
{
	return 0;
}

long frame_driver_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	unsigned long size;

	if (cmd == GET_FRAME_BUFFER) {
		pr_info("buffer having index %d is filled", (int)arg);
		I_FLAG = false;
		vbuf = p;
		p->frame_no = (int)arg;
		I_FLAG = true;
		return 0;
	}

	copy_from_user(&frame_data, (struct yuv_data *) arg, sizeof(struct yuv_data));
	pr_info("%s: frame rate = %d, frame count = %d\n", __func__, frame_data.framerate, frame_data.framecount);
	pr_info("%s: frame width = %d, frame height = %d\n", __func__, frame_data.width, frame_data.height);
	size = frame_data.width * frame_data.height << 1;
	p = kmalloc(sizeof(struct yuv_frame), GFP_KERNEL);
	p->data = kmalloc(size, GFP_KERNEL);
	return 0;
}

static void vm_close(struct vm_area_struct *vma)
{
	pr_info("vm_close\n");
}

static vm_fault_t vm_fault(struct vm_fault *vmf)
{
	char *buf;
	struct page *page;
	unsigned long offset;  

	pr_info("vm_fault\n");
	offset = (unsigned long)(vmf->address - vmf->vma->vm_start);
	printk("%lu", offset);
	buf = (char *)vmf->vma->vm_private_data;
	printk("%p", buf);
	if (buf) {
		page = virt_to_page(buf + offset);
		get_page(page);
		vmf->page = page;	
	}	
	return 0;
}

static void vm_open(struct vm_area_struct *vma)
{
	pr_info("vm_open\n");
}

static struct vm_operations_struct vm_ops = {
	.fault		= vm_fault,
	.open		= vm_open,
	.close		= vm_close,	
};

static int frame_driver_mmap(struct file *filp, struct vm_area_struct *vma)
{
	pr_info("mmap\n");
	vma->vm_ops = &vm_ops;
	vma->vm_flags |= VM_DONTEXPAND | VM_DONTDUMP;
	vma->vm_private_data = p->data;
	printk("%p", p->data);
	vm_open(vma);
	return 0;
}


static const struct file_operations fops = {
	.owner		= THIS_MODULE,
	.open		= frame_driver_open,
	.release	= frame_driver_release,
	.mmap		= frame_driver_mmap,
	.unlocked_ioctl = frame_driver_ioctl,
};

static int __init frame_driver_init(void)
{
	pr_info("%s: Inserting driver\n", __func__);
	if (alloc_chrdev_region(&device, 0, 1, name) < 0) {
		pr_err("device registration failed..\n");
		return -1;
	}
	pr_info("Device: %s, Major number: %d\n", name, MAJOR(device));
	class = class_create(THIS_MODULE, name);
	device_create(class, NULL, device, NULL, name);
	cdev = cdev_alloc();
	cdev_init(cdev, &fops);
	cdev_add(cdev, device, 1);
	return 0;
}


static void __exit frame_driver_exit(void)
{
	pr_info("%s: Removing driver\n", __func__);

	if (p) {
		kfree(p);
		kfree(p->data);
	}

	cdev_del(cdev);
	device_destroy(class, device);
	class_destroy(class);
	unregister_chrdev_region(device, 1);
}

module_init(frame_driver_init);
module_exit(frame_driver_exit);
