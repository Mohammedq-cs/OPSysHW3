#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/ioctl.h>
#include <linux/errno.h>
#include "message_slot.h"

MODULE_LICENSE("GPL");

channel *devices[257];

static int device_open(struct inode *inode, struct file *file){
    unsigned int minor;
    fdata *data;
    minor = iminor(inode);
    data = kmalloc(sizeof(fdata*), GFP_KERNEL);
    if (data == NULL){
        printk("file data malloc has failed in device_open. file: %p\n", file);
        return -ENOMEM;
    }
    data->minor_number = minor;
    data->current_channel_id = 0;
    file->private_data = (void*) data;
    printk("File opened!\n");

    return SUCCESS;

}

static int device_release(struct inode* inode, struct file *file){
    kfree(file->private_data);
    return SUCCESS;

}



//read function that reads the last written message on the device
static ssize_t device_read(struct file* file, char __user *buffer, size_t length, loff_t *offset){
    fdata *data;
    channel *ch;
    data = (fdata*) file->private_data;
    if (buffer == NULL || data->current_channel_id == 0){
        return -EINVAL;
    }
    ch = data->current_channel;
    if (ch->message_size == 0){
        return -EWOULDBLOCK;
    }
    if (length < ch->message_size){
        return -ENOSPC;
    }
    if (copy_to_user(buffer, ch->message, ch->message_size) != 0){
        return -EIO;
    }

    return ch->message_size;
}

static ssize_t device_write(struct file* file, const char __user* buffer, size_t length, loff_t *offset){
    fdata *data;
    channel *ch;
    char msg[BUFFER_LEN];
    int i;
    data = file->private_data;
    ch = (channel*) data->current_channel;


    if (buffer == NULL){
        return -EINVAL;
    }
    if (data->current_channel_id == 0){
        return -EINVAL;
    }
    if (length == 0 || length > BUFFER_LEN){
        return -EMSGSIZE;
    }
    if (copy_from_user(msg, buffer, length) != 0){
        return -EIO;
    }

    for (i = 0; i < BUFFER_LEN && i < length; ++i){
        ch->message[i] = msg[i];
    }
    ch->message_size = (int) length;
    return i;


}

static long device_ioctl(struct file* file, unsigned int ioctl_command_id, unsigned long ioctl_param){
    fdata *data;
    unsigned int minor;
    channel *ch;
    channel *tmp;
    channel *prev;
    if (ioctl_command_id != MSG_SLOT_CHANNEL || ioctl_param == 0){
        return -EINVAL;
    }
    data = file->private_data;
    minor = data->minor_number;
    if (ioctl_param == data->current_channel_id){
        return SUCCESS;
    }
    ch = (channel*) devices[minor];
    //we still have no channels, so we create a new one.
    if (ch == NULL){
        ch = kmalloc(sizeof(channel), GFP_KERNEL);
        if (ch == NULL){
            printk(KERN_ERR "channel allocation has failed.\n");
            return -ENOMEM;
        }
        ch->channel_id = ioctl_param;
        ch->message_size = 0;
        ch->next = NULL;
        data->current_channel_id = ioctl_param;
        data->current_channel = ch;
        devices[minor] = (channel*) ch;
        return SUCCESS;

    }
    //we have to
    else{
        tmp = ch;
        while (tmp != NULL){
            if (tmp->channel_id == ioctl_param){
                data->current_channel = (channel*) tmp;
                data->current_channel_id = ioctl_param;
                return SUCCESS;
            }
            prev = tmp;
            tmp = tmp->next;
        }
        tmp = kmalloc(sizeof(channel), GFP_KERNEL);
        if (tmp == NULL){
            printk(KERN_ERR "channel allocation has failed.\n");
            return -ENOMEM;
        }
        tmp->channel_id = ioctl_param;
        tmp->message_size = 0;
        tmp->next = NULL;
        prev->next = tmp;
        data->current_channel_id = ioctl_param;
        data->current_channel = tmp;
    }
    return SUCCESS;




}
//---------------DEVICE SETUP---------------

static const struct file_operations fops = {
        .owner = THIS_MODULE,
        .open = device_open,
        .read = device_read,
        .write = device_write,
        .unlocked_ioctl = device_ioctl,
        .release = device_release
};

static int __init slot_init(void){
    int ret;
    int i;
    ret = register_chrdev(MAJ_NUMBER, NAME, &fops);
    if (ret < 0){
        printk(KERN_ERR "device registration failed for %s, number: %d\n", NAME, MAJ_NUMBER);
        return ret;
    }
    for (i = 0; i < 257; ++i){
        devices[i] = NULL;
    }
    printk("Device has successfully registered.\n");
	return SUCCESS;
}

static void __exit slot_cleanup(void){
    int i;
    channel *tmp, *head;
    for (i = 0; i < 257; ++i){
        channel *ch = devices[i];
        head = ch;
        while (head != NULL){
            tmp = head;
            head = head->next;
            kfree(tmp);
        }
    }
    unregister_chrdev(MAJ_NUMBER, NAME);
    printk("Device successfully unregistered.\n");
}

module_init(slot_init);
module_exit(slot_cleanup);

