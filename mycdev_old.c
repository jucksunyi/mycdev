/**************************************************************
name: my character device driver
desc: this is a character device drive frame .the character device is not a real device ,it use mem simulate a device.
param: cdev_major (major device number)
return: null
Author: muke
Date: 2017-7-13
Modify: 2018-06-05
this file  nedd    mknod  insmod mycdev.ko  &&   mknod /dev/mycdev c 241 0
*******************************************************************/

#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/cdev.h>

#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/io.h>
#include <linux/of.h>
#include "mycdev.h"

#define __MYDEBUG__
#ifdef __MYDEBUG__
#define MYDEBUG(format,...) printk("File: "__FILE__", Line: %05d: "format"\n", __LINE__, ##__VA_ARGS__)
#else
#define MYDEBUG(format,...)
#endif

#define SIZE_BUF 33
static int my_major = MYCDEV_MAJOR;


module_param(my_major,int,S_IRUGO);

struct mycdev *mycdevp;
struct cdev ccdev;


static int my_read_register_new(unsigned long addr, size_t size,
							unsigned long *value)  
{

    void __iomem *p;
	p = ioremap(addr, size);
	if (p) {
		*value = ioread32(p);
		MYDEBUG("**point value%x", *value);
	}

	iounmap(p);

	return 0;
}

static int atoi(char *p, char *p1)  
{
	int temp = 0;  
  
    while(*p != '\0')  
    {  
        if('0' <= *p && *p <= '9')  
        {  
            *p1 = *p;  
            //printf("%c",*p1);  
            temp = temp * 10 + (*p1 - '0');  
            p1++;  
        }  
  
        p++;  
    }  
      
    return temp;  
    //printf("%s\n",*p1);  
}  

int mycdev_open(struct inode *inode ,struct file *filp)
{
	int num = MINOR(inode->i_rdev);
	if(num > MYCDEV_NR_DEVS)
		return -ENODEV;
	filp->private_data = &mycdevp[num];
	
	return 0;
}

int mycdev_release(struct inode *inode,struct file *filp){

	return 0;
}

static ssize_t mycdev_read(struct file *filp ,char __user *buf,size_t size,loff_t *pos)
{
	unsigned long position = *pos;
	unsigned int count = size;
	int i,  ret = 0;
	struct mycdev *dev = filp->private_data;

	unsigned long value_tmp = 0;
    unsigned long addr_tmp = 0;
	char *kbuf = NULL;

	MYDEBUG("the kbuf in fun-read  is :%s", dev->data);

	addr_tmp = atoi(dev->data, mycdevp->data);
	my_read_register_new(addr_tmp, 4, &value_tmp);
	MYDEBUG("Value of addr: 0x%x is 0x%x", addr_tmp, value_tmp);
	
	sprintf(dev->data, "%lu", value_tmp);
	
	if(position > MYCDEV_SIZE){
	        printk(KERN_ALERT "read out of range!\n");
		return 0;
	}
	if(position + count > MYCDEV_SIZE )
		count = MYCDEV_SIZE - position;

	if(copy_to_user(buf,(void *)(dev->data + position),count))
	{
		ret = -EFAULT;
	}else
	{
		MYDEBUG("the buf copy to user is :%d", atoi(buf, mycdevp->data));
		*pos += count;
		ret = count;
		MYDEBUG("read SUCCESS %d bytes from %ld.",count,position);
	}

	return ret;

}

static ssize_t mycdev_write(struct file *filp, const char __user *buf,
							size_t size, loff_t *pos )
{
	unsigned long position = *pos;
	unsigned int count = size;
	int ret = 0, i;
	struct mycdev *dev = filp->private_data;
	
	if( position > MYCDEV_SIZE){
		printk(KERN_ALERT "write out of range!\n");
		return 0;
	}
	
	if(count + position > MYCDEV_SIZE){
		count = MYCDEV_SIZE - position;
		
	}
	
	if(copy_from_user((void *)(dev->data + position),buf,count))
	{	
		ret = -EFAULT;
	}
	else
	{
		MYDEBUG("the kbuf copy from user is :%s", dev->data);
		*pos += count;
		ret = count;
		MYDEBUG("written %d bytes from %ld.",count,position);
	}

	return ret;
}

static loff_t mycdev_llseek(struct file *filp , loff_t offset, int mode)
{
	loff_t newpos;
	
	switch(mode){
		case 0: /*seek_set*/
			newpos = offset;
			break;
		case 1: /*seek_cur*/
			newpos = filp->f_pos + offset;
			break;
		case 2: /*seek_end*/
			newpos = MYCDEV_SIZE -1 +offset;
			break;
		default:
			printk(KERN_ALERT "seek mode error!\n");
			return -EINVAL;

	}
	if((newpos < 0) || (newpos > MYCDEV_SIZE)){
		printk(KERN_ALERT "wrong postion!\n");
		return -EINVAL;
	}
	filp->f_pos = newpos;

	return newpos;
}
	

/* reload file_operators struct */

static const struct file_operations mycdev_fops = 
{

	.owner = THIS_MODULE,
	.llseek = mycdev_llseek,
	.read = mycdev_read,
	.write = mycdev_write,
	.open = mycdev_open,
	.release = mycdev_release,

};

static int mycdev_init(void)
{
        int result;
	int i;
	dev_t devnum = MKDEV(my_major,0);

	MYDEBUG("mycdev_init start!");
	
// 	if(my_major){
// 
// 		result = register_chrdev_region(devnum ,2 ,"mycdev");
// 
// 	}else
// 	{
		result = alloc_chrdev_region(&devnum,0,2,"mycdev");
		my_major = MAJOR(devnum);
		printk(KERN_INFO "register_chrdev success... mymajor = %d.\n", my_major);
			
//	}

	if(result < 0 )
		return result;
	
	cdev_init(&ccdev,&mycdev_fops);
	ccdev.owner = THIS_MODULE;
	ccdev.ops = &mycdev_fops;

	cdev_add(&ccdev,MKDEV(my_major,0),MYCDEV_NR_DEVS);

	mycdevp = kmalloc(MYCDEV_NR_DEVS*sizeof(struct mycdev),GFP_KERNEL);

	if(!mycdevp){

		result = -ENOMEM;
		goto fail_malloc;
	}

	memset(mycdevp,0,sizeof(struct mycdev)*MYCDEV_NR_DEVS);

	for(i=0;i < MYCDEV_NR_DEVS;i++)
	{
		mycdevp[i].size = MYCDEV_SIZE;
		mycdevp[i].data = kmalloc(MYCDEV_SIZE,GFP_KERNEL);
		memset(mycdevp[i].data, 0,MYCDEV_SIZE);
	}
	
	return 0;
	
	fail_malloc:
	unregister_chrdev_region(devnum, 2);
	return result;
}

static void mycdev_exit(void ){
	
	int i ;
	cdev_del(&ccdev);
	for(i =0;i<MYCDEV_NR_DEVS;i++){

		kfree(mycdevp[i].data);

	}

	kfree(mycdevp);
	unregister_chrdev_region(MKDEV(my_major,0),2);

}	


MODULE_AUTHOR("***");
MODULE_LICENSE("GPL");


module_init(mycdev_init);
module_exit(mycdev_exit);


