/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @editor Sonal Tamrakar, ECEN 5713 AESD 
 * @credit Linux Device Drivers 3rd Edition, Chapter 3 Char Drivers
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/proc_fs.h>
#include <linux/fcntl.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/seq_file.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h> // file_operations

#include "aesd_ioctl.h"
#include "aesdchar.h"



int aesd_major =   0; // use dynamic major, so these don't matter
int aesd_minor =   0;

MODULE_AUTHOR("Sonal Tamrakar"); /** TODO: fill in your name -> DONE **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

/* When the device is opened for init, the kernel provides an inode
with a pointer to the cdev structure, which is inode->i_cdev. The driver
will want to get access of the larger aesd_dev structure that contains 
the cdev field. container_of essentially helps convert the cdev pointer into
the aesd_dev pointer. The aesd_dev structure is then saved into the 
filp->private_data for later use. 
struct cdev *i_cdev is the kernel's internal structure that represents
char devices. */
int aesd_open(struct inode *inode, struct file *filp)
{
    struct aesd_dev *dev; /* device information */
    PDEBUG("open");
    
    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = dev; /*for other methods*/
    return 0; /* success */
}


int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    /**
     * TODO: handle release. The basic form of aesdchar has no hardware to shutdown
     * aesd_release() doesn't need to deallocate anything as open() didn't allocate
     * anything for filp->private_data
     */
    return 0;
}

/**
 * "The llseek method is used to change the current read/write position
 * in a file, and the new position is returne as a (positive) return value.
 * The loff_t parameter is a "long offset" and is at least 64 bits wide 
 * even on 32-bit platforms. Errors are signaled by a negative return value.
 * If this function pointer is NULL, seek calls will modify the position
 * counter in the file structure in potentially unpredictable ways."
 * Linux Device Drivers Chapter 3, J. Corbet, A Rubini, G. Kroah-Hartman
 */
loff_t aesd_llseek(struct file *filp, loff_t offset, int whence)
{
    ssize_t retval;
    loff_t fsl;
    loff_t circbuffsize=0;
    struct aesd_dev *dev = NULL;
    struct aesd_buffer_entry *entry = NULL;
    uint8_t stread;
    if (filp == NULL)
    {
        retval = -EINVAL;
        PDEBUG("file pointer filp NULL, returning %zu", retval);
        goto errout;
    }

    dev = filp->private_data;

    PDEBUG("entered seek with offset = %lld and whence = %d", offset, whence);

    if (mutex_lock_interruptible(&dev->bufferlock))
    {
        retval = -ERESTARTSYS;
        PDEBUG("aesd_llseek: mutex lock interruptible , returning %zu", retval);
        goto errout;
    }

    AESD_CIRCULAR_BUFFER_FOREACH(entry, &(aesd_device.circular), stread)
    {
        if(entry->buffptr != NULL)
            circbuffsize += entry->size;
    }

    //need to get the total size of all content of the circular buffer
    fsl = fixed_size_llseek(filp, offset, whence, circbuffsize);
    if (fsl == -EINVAL)
    {
        retval = fsl;
        PDEBUG("fixed_size_llseek() returned -EINVAL. %zu", retval);
        goto unlockout;
    }
    retval = fsl;
    //filp->f_pos += fsl;

    unlockout:
    mutex_unlock(&dev->bufferlock);
    errout:
    return retval;
}


/**
 * Adjust the file offset (f_pos) parameter of @param filp based on the location specified by
 * @param write_cmd (the zero referenced command to locate) and @param write_cmd_offset
 * (the zero referenced offset into the command) 
 * @return 0 if successful, negative if error occurred:
 *      -ERESTARTSYS if mutex could not be obtained
 *      -EINVAL if write command or write_cmd_offset was out of range.
 */
static long aesd_adjust_file_offset(struct file *filp, unsigned int write_cmd, unsigned int write_cmd_offset)
{
    ssize_t retval;
    struct aesd_dev *dev = NULL;
    struct aesd_buffer_entry *entry = NULL;
    ssize_t circbuffsize = 0;
    uint8_t index;
    uint8_t i=0;
    if (filp == NULL)
    {
        retval = -EINVAL;
        PDEBUG("file pointer filp NULL, returning %zu", retval);
        return retval;
    }

    if (write_cmd > AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED-1)
    {
        retval = -EINVAL;
        PDEBUG("write_cmd_offset was out of range. %zu", retval);
        return retval;
    }


    dev = filp->private_data;


    if (mutex_lock_interruptible(&dev->bufferlock))
    {
        retval = -ERESTARTSYS;
        PDEBUG("aesd_adjust_file_offset mutex lock interruptible, returning %zu", retval);
        return retval;
    }    

    AESD_CIRCULAR_BUFFER_FOREACH(entry, &(aesd_device.circular), index)
    {
        if(entry->buffptr != NULL)
            circbuffsize++;
    }

    PDEBUG("Total entries are %ld", circbuffsize);

    if (write_cmd > circbuffsize)
    {
        retval = -EINVAL;
        PDEBUG("write_cmd offset was out of range. %zu", retval);
        goto errout;
    }

    if(write_cmd_offset > dev->circular.entry[write_cmd].size-1)
    {
        retval = -EINVAL;
        PDEBUG("write_cmd_offset was out of range. %zu", retval);
        goto errout;
    }

    while (i < write_cmd)
    {
        filp->f_pos += dev->circular.entry[i].size;
        i++;
    }

    filp->f_pos += write_cmd_offset;
    retval = 0;


    errout:
    mutex_unlock(&dev->bufferlock);
    return retval;

}

long aesd_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    ssize_t retval; 
    if (filp == NULL)
    {
        retval = -EINVAL;
        PDEBUG("file pointer filp NULL, returning %zu", retval);
        goto errout;
    }
    
    switch(cmd) {
        case AESDCHAR_IOCSEEKTO:
        {
            struct aesd_seekto seekto;
            if (copy_from_user(&seekto, (const void __user *)arg, sizeof(seekto)) != 0) {
                retval = EFAULT;
            } else {
                retval = aesd_adjust_file_offset(filp, seekto.write_cmd, seekto.write_cmd_offset);
            }
            break;
        }

        default:
            return -ENOTTY;
    }

    errout:
    return retval;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    /* filp- file pointer: private_data member can be used to get aesd_dev
        buff - buffer to fill. Can we access this buffer directly? No, use copy_to_user, read 
        from device basically. count - max number of bytes to write to buff. You 
        may want/need to write less than this*/
    //*buf is the buffer to fill, copy from kernel space to user space
    //count - max number of bytes to write ("read into") buff, 
    // May want/need to write less than this
    // also reading the f_pos which is the char_offset??
    ssize_t retval = 0;
    ssize_t read_bytes;
    size_t entry_offset_rd;
    struct aesd_dev *dev = NULL;
    struct aesd_buffer_entry *entry = NULL;
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);

    if (filp == NULL || buf == NULL)
    {
        retval = -EINVAL;
        PDEBUG("file pointer filp NULL, returning %zu", retval);
        goto errout;
    }

    dev = filp->private_data; //device pointer acquired here

    if (mutex_lock_interruptible(&dev->bufferlock))
    {
        retval = -ERESTARTSYS;
        PDEBUG("aesd_read: mutex lock interruptible, returning %zu", retval);
        goto errout;
    }
    
    entry = aesd_circular_buffer_find_entry_offset_for_fpos(&(dev->circular), *f_pos, &entry_offset_rd);

    if (entry == NULL)
    {
        retval = 0;
        PDEBUG("aesd_circular_buffer_find_entry_offset_for_fpos NULL return %zu", retval);
        goto out; //mutex is held so can't goto errout
    }

    //check/get the minimum between count and the size minus the entry offset
    read_bytes = ((entry->size - entry_offset_rd) < count) ? (entry->size - entry_offset_rd) : count;

    if(copy_to_user(buf, (entry->buffptr + entry_offset_rd), read_bytes))
    {
        retval = -EFAULT;
        PDEBUG("copy_to_user non-negative value, returning %zu", retval);
        goto out; //mutex is held so can't goto errout
    }

    //point to the next offset
    *f_pos += read_bytes;
    retval = read_bytes;
    goto out;

    errout:
    return retval;

    out:
    mutex_unlock(&dev->bufferlock);
    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    //ignore *f_pos???
    // either append to the command being written when there's no
    //newline received or
    //write to the command buffer when a newline is received
    //destination is the circular buffer
    ssize_t retval = -ENOMEM;
    size_t write_bytes = 0;
    size_t i = 0;
    struct aesd_dev *dev = NULL;
    const char* entry_to_free = NULL; 
    char* buftemp = NULL; 
    char *newline_ptr = NULL;
    bool nline_found = false;
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);

    if (filp == NULL || buf == NULL)
    {
        retval = -EINVAL;
        PDEBUG("file pointer filp NULL, returning %zu", retval);
        return retval;
    }

    dev = filp->private_data;

    //kmalloc required count
    buftemp = kmalloc(count, GFP_KERNEL);
    if (buftemp == NULL)
    {
        PDEBUG("kmalloc allocation failed.");
        retval = -ENOMEM;
        return retval;
    }

    //read from user into kernel, destination is buftemp
    if (copy_from_user(buftemp, buf, count)) {
        PDEBUG("copy_from_user failed");
		retval = -EFAULT;
		goto free_out;
    }

    if (mutex_lock_interruptible(&dev->bufferlock))
    {
        retval = -ERESTARTSYS;
        PDEBUG("aesd_write: mutex lock interruptible, returning %zu", retval);
        goto free_out;
    }
    
    for(i = 0; i < count; i++)
    {
        if(buftemp[i] == '\n')
        {
            newline_ptr = &buftemp[i];
            nline_found = true;
            break;
        }
    }

    write_bytes = (newline_ptr) ? (newline_ptr -&buftemp[0]+1) : count;
    
    dev->entry.buffptr = krealloc(dev->entry.buffptr, dev->entry.size+write_bytes, GFP_KERNEL);
    if(dev->entry.buffptr == NULL)
    {
        PDEBUG("krealloc failed here.");
        retval = -ENOMEM;
        goto out;
    }

    memcpy((char *)(dev->entry.buffptr+dev->entry.size), buftemp, write_bytes);
    dev->entry.size += write_bytes;


    if (nline_found)
    {
        entry_to_free = aesd_circular_buffer_add_entry(&dev->circular, &dev->entry);
        if (entry_to_free != NULL)
        {
            kfree(entry_to_free);
        }
        //entry completed, reset buffptr and size
        dev->entry.buffptr = NULL;
        dev->entry.size = 0;
    }

    retval = count;
    goto out;


    out:
    mutex_unlock(&dev->bufferlock);

    free_out:
    kfree(buftemp);
    return retval;
}


// function pointers for the file operations
struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
    .llseek =   aesd_llseek,
    .unlocked_ioctl = aesd_unlocked_ioctl,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    //sets up the character device, and intializes 
    // it with cdev_init defining the file operations
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    //cdev_add adds the device into the linux kernel
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}



int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    // Allocation of a character device region
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    //Store the major device number
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    // Clears the aesd_device structure by setting all its bytes to zero.
    memset(&aesd_device,0,sizeof(struct aesd_dev));

    mutex_init(&aesd_device.bufferlock);
    aesd_circular_buffer_init(&aesd_device.circular);
    aesd_device.entry.buffptr = NULL;
    aesd_device.entry.size = 0;

    // setus up the character device (cdev) 
    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    struct aesd_buffer_entry *del_entry = NULL;
    uint8_t index;
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);


     AESD_CIRCULAR_BUFFER_FOREACH(del_entry, &(aesd_device.circular), index)
     {
        if(del_entry->buffptr)
            kfree(del_entry->buffptr);
     }

    

    /**
     * TODO: cleanup AESD specific poritions here as necessary
     * Not dynamically allocating anything so shouldn't need anything here
     */
    mutex_destroy(&aesd_device.bufferlock);
    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
