#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <linux/uaccess.h>  /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
#include <linux/slab.h>
#include <linux/errno.h>
#include "message_slot.h"
MODULE_LICENSE("GPL");

channel *devices[257];


//================== DEVICE FUNCTIONS ===========================
static int device_open( struct inode* inode,
                        struct file*  file )
{
    unsigned int minor;
    fileData *data;
    minor = iminor(inode);
    data = kmalloc(sizeof(fileData*), GFP_KERNEL);
    if (data == NULL){
        printk("file data malloc has failed in device_open. file: %p\n", file);
        return -ENOMEM;
    }
    data->minor_number = minor;
    data->current_channel_id = 0;
    file->private_data = (void*) data;

    return SUCCESS;
}

//---------------------------------------------------------------
static int device_release( struct inode* inode,
                           struct file*  file)
{
    kfree(file->private_data);
    return SUCCESS;
}

//---------------------------------------------------------------
// a process which has already opened
// the device file attempts to read from it
static ssize_t device_read( struct file* file,
                            char __user* buffer,
                            size_t       length,
                            loff_t*      offset )
{
    fileData *fData;
    channel *ch;
    fData = (fileData*) file->private_data;
    if (buffer == NULL || fData->current_channel_id == 0){
        return -EINVAL;
    }
    ch = fData->current_channel;
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

//---------------------------------------------------------------
// a processs which has already opened
// the device file attempts to write to it
static ssize_t device_write( struct file*       file,
                             const char __user* buffer,
                             size_t             length,
                             loff_t*            offset)
{
    fileData *data;
    channel *ch;
    char msg[BUFFER_LEN];
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

    for (i = 0; i < BUFFER_LEN && i < length; i++){
        ch->message[i] = msg[i];
    }
    ch->message_size = (int) length;
    return i;
  
}

//----------------------------------------------------------------
static long device_ioctl( struct   file* file,
                          unsigned int   ioctl_command_id,
                          unsigned long  ioctl_param )
{
    fileData *data;
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
    if (ch == NULL)
    {
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
        while (tmp != NULL) {
            if (tmp->channel_id == ioctl_param) {
                data->current_channel = (channel *) tmp;
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

//==================== DEVICE SETUP =============================

// This structure will hold the functions to be called
// when a process does something to the device we created
struct file_operations Fops =
{
  .owner	  = THIS_MODULE, 
  .read           = device_read,
  .write          = device_write,
  .open           = device_open,
  .unlocked_ioctl = device_ioctl,
  .release        = device_release,
};

//---------------------------------------------------------------
// Initialize the module - Register the character device
static int __init simple_init(void)
{
    int rc;
    int i;
    rc = register_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME, &Fops);
    if (rc < 0){
        printk(KERN_ERR "device registration failed for %s with number: %d\n", DEVICE_RANGE_NAME, MAJOR_NUM);
        return rc;
    }
    for (i = 0; i < 257; i++){
        devices[i] = NULL;
    }
    printk("Registeration is successful.\n");
	return SUCCESS;
}

//---------------------------------------------------------------
static void __exit simple_cleanup(void)
{
    int i;
    channel *tmp, *head;
    for (i = 0; i < 257; ++i)
    {
        channel *ch = devices[i];
        head = ch;
        while (head != NULL)
        {
            tmp = head;
            head = head->next;
            kfree(tmp);
        }
    }
  // Unregister the device
  // Should always succeed
  unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

//---------------------------------------------------------------
module_init(simple_init);
module_exit(simple_cleanup);

//========================= END OF FILE =========================
