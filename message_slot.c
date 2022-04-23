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

channel *channelHeads[257];

//================== DEVICE FUNCTIONS ===========================
static int device_open(struct inode *inode, struct file *file){
    unsigned int minor;
    fileData *fileData;
    minor = iminor(inode);
    fileData = kmalloc(sizeof(fileData*), GFP_KERNEL);
    if (fileData == NULL){
        printk("file fileData malloc has failed in device_open. file: %p\n", file);
        return -ENOMEM;
    }
    fileData->minorNum = minor;
    fileData->currentChannelId = 0;
    file->private_data = (void*) fileData;
    printk("Success in opening the file");
    return SUCCESS;
}

static int device_release(struct inode* inode, struct file *file){
    kfree(file->private_data);
    return SUCCESS;
}

static ssize_t device_read(struct file* file, char __user *buffer, size_t length, loff_t *offset){
    fileData *fileData;
    channel *headChannel;
    fileData = (fileData*) file->private_data;
    if (buffer == NULL || fileData->currentChannelId == 0){
        return -EINVAL;
    }
    headChannel = fileData->currentChannel;
    if (headChannel->messageSize == 0){
        return -EWOULDBLOCK;
    }
    if (length < headChannel->messageSize){
        return -ENOSPC;
    }
    if (copy_to_user(buffer, headChannel->message, headChannel->messageSize) != 0){
        return -EIO;
    }
    printk("success in reading message, message len is %d\n", headChannel->message_size);
    return headChannel->messageSize;
}

static ssize_t device_write(struct file* file, const char __user* buffer, size_t length, loff_t *offset){
    fileData *fileData;
    channel *headChannel;
    char msg[BUFFER_LEN];
    int i;
    fileData = file->private_data;
    headChannel = (channel*) fileData->currentChannel;

    if (buffer == NULL){
        return -EINVAL;
    }
    if (fileData->currentChannelId == 0){
        return -EINVAL;
    }
    if (length == 0 || length > BUFFER_LEN){
        return -EMSGSIZE;
    }
    if (copy_from_user(msg, buffer, length) != 0){
        return -EIO;
    }

    for (i = 0; i < BUFFER_LEN && i < length; ++i){
        headChannel->message[i] = msg[i];
    }
    headChannel->messageSize = (int) length;
    printk("success in writing message, message len is %d\n", i);
    return i;
}

static long device_ioctl(struct file* file, unsigned int ioctl_command_id, unsigned long ioctl_param){
    fileData *fileData;
    channel *headChannel;
    channel *prevChannel;
    channel *tmpChannel;
    unsigned int minor;

    if (ioctl_command_id != MSG_SLOT_CHANNEL || ioctl_param == 0){
        return -EINVAL;
    }
    fileData = file->private_data;
    minor = fileData->minorNum;
    if (ioctl_param == fileData->currentChannelId){
        return SUCCESS;
    }
    headChannel = (channel*) channelHeads[minor];
    if (headChannel == NULL){
        headChannel = kmalloc(sizeof(channel), GFP_KERNEL);
        if (headChannel == NULL){
            printk(KERN_ERR "channel allocation has failed.\n");
            return -ENOMEM;
        }
        headChannel->channelId = ioctl_param;
        headChannel->messageSize = 0;
        headChannel->next = NULL;
        fileData->currentChannelId = ioctl_param;
        fileData->currentChannel = headChannel;
        channelHeads[minor] = (channel*) headChannel;
        printk("success creating a head channel");
        return SUCCESS;
    }
    tmpChannel = headChannel;
    while (tmpChannel != NULL){
        if (tmpChannel->channelId == ioctl_param){
            fileData->currentChannel = (channel*) tmpChannel;
            fileData->currentChannelId = ioctl_param;
            printk("success in finding the channel");
            return SUCCESS;
        }
        prevChannel = tmpChannel;
        tmpChannel = tmpChannel->next;
    }
    tmpChannel = kmalloc(sizeof(channel), GFP_KERNEL);
    if (tmpChannel == NULL){
        printk(KERN_ERR "channel allocation has failed.\n");
        return -ENOMEM;
    }
    tmpChannel->channelId = ioctl_param;
    tmpChannel->messageSize = 0;
    tmpChannel->next = NULL;
    prevChannel->next = tmpChannel;
    fileData->currentChannelId = ioctl_param;
    fileData->currentChannel = tmpChannel;
    
    printk("success creating a tail channel");
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

static int __init simple_init(void){
    int ret;
    int i;
    ret = register_chrdev(MAJ_NUMBER, NAME, &fops);
    if (ret < 0){
        printk(KERN_ERR "device registration failed for %s, number: %d\n", NAME, MAJ_NUMBER);
        return ret;
    }
    for (i = 0; i < 257; ++i){
        channelHeads[i] = NULL;
    }
    printk("Device has successfully registered.\n");
	return SUCCESS;
}

static void __exit slot_cleanup(void){
    int i;
    channel *tmp, *head;
    for (i = 0; i < 257; ++i){
        channel *ch = channelHeads[i];
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

module_init(simple_init);
module_exit(slot_cleanup);

