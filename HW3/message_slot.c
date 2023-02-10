//
// Created by shira on 16/12/2022.
//


#undef __KERNEL__
#define __KERNEL__

#undef MODULE
#define MODULE

#include "message_slot.h"
#include <linux/module.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/printk.h>
#include <linux/slab.h>

#define SUCCESS 0
#define DEVICE_NAME "message_slot"


typedef struct file File;
typedef struct inode Inode;

typedef struct Node{
    unsigned int channelID;
    char *message;
    int message_len;
    struct Node* next;

}channelNode;


channelNode* minors[257];
int minor = -1;

void delete_list(channelNode* head) {
    if(head != NULL){
        delete_list(head->next);
        kfree(head->message);
        kfree(head);
    }
}

void appendNewChannel(channelNode** head_p, int channelID){ // https://www.geeksforgeeks.org/insertion-in-linked-list/
    channelNode *newChannelNode = newChannelNode= (channelNode*)kmalloc(sizeof(channelNode), GFP_KERNEL);
    newChannelNode->channelID = channelID;
    newChannelNode->next = (*head_p);
    (*head_p) = newChannelNode;
}

int lookupChannelID(unsigned int channelID) { // https://www.geeksforgeeks.org/search-an-element-in-a-linked-list-iterative-and-recursive/
    channelNode *curr;
    curr = minors[minor];
    while (curr != NULL) {
        if (curr->channelID == channelID)
            return 1;
        curr = curr->next;
    }
    return 0;
}

channelNode* getChannelNodeByCahnnelID(unsigned int channelID) { // https://www.geeksforgeeks.org/search-an-element-in-a-linked-list-iterative-and-recursive/
    channelNode* curr = minors[minor];
    while (curr != NULL) {
        if (curr->channelID == channelID) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

void regiserChannelIfNeeded(unsigned int channelID) {
    if (!lookupChannelID(channelID)) {
        appendNewChannel(&minors[minor], channelID);
    }
}

void attachMessageToChannel(char* message, unsigned long channelId, int len) {
    channelNode* channel = getChannelNodeByCahnnelID(channelId);
    if (channel->message != NULL) {
        kfree(channel->message);
    }
    channel->message = message;
    channel->message_len = len;
}

void freeAllMinors(void) {
    int i;
    for(i=0; i<256; i++){
        delete_list(minors[i]);
    }
}


static int device_open(Inode* inode, struct file* file) { //function general build taken from Eran's code
    channelNode* newChannelNodeHead;
    minor = iminor(inode);
    if (minors[minor] == NULL) { // first time opened with this minor number, need to initialize channels linked list
        newChannelNodeHead = (channelNode*)kmalloc(sizeof(channelNode), GFP_KERNEL);
        minors[minor] = newChannelNodeHead;
    }
    return SUCCESS;
}

static ssize_t device_read(File* file, char __user* buffer, size_t length, loff_t* offset) { //function general build taken from Eran's code
    ssize_t i;
    char *message;
    channelNode *channelNode;
    if (file->private_data == NULL) { // handle error: no channel ha been set on file
        return -EINVAL;
    }
    channelNode = getChannelNodeByCahnnelID((unsigned long)file->private_data);
    if (channelNode->message == NULL) { // handle error: filed to allocate memory
        return -EWOULDBLOCK;
    }
    message = channelNode->message;
    if (length < channelNode->message_len) { // handle error: provided buffer length is too small to hold last message
        return -ENOSPC;
    }
    for (i = 0; i < length && i < BUF_LEN; ++i) { // reading message from user buffer char by char
        if (put_user(message[i], &buffer[i]) < 0) { // handle error: failed to execute put_user() method
            return -ENOSPC;
        }
    }
    if (length-i != 0) { // handle error: didn't finish reading message to kernel
        return -EWOULDBLOCK;
    }
    return i;
}

static ssize_t device_write(File* file, const char __user* buffer, size_t length, loff_t* offset) { //function general build taken from Eran's code
    ssize_t i;
    char* message;
    unsigned long channelID;
    if (file->private_data == NULL) { // handle error: no channel ha been set on file
        return -EINVAL;
    }
    if (length == 0 || length > 128) { // handle error: message is empty or to long
        return -EMSGSIZE;
    }
    message = (char*)kmalloc(sizeof(char)*BUF_LEN, GFP_KERNEL);
    if (message == NULL) { // handle error: filed to allocate memory
        return -ENOMEM;
    }
    channelID = (unsigned long)file->private_data;
    for( i = 0; i < length && i < BUF_LEN; ++i ) { // writing message from user buffer char by char
        if (get_user(message[i], &buffer[i]) < 0) {  // handle error: failed to execute get_user() method
            return -ENOSPC;
        }
    }
    if (length-i != 0){ // handle error: didn't finish writing message to kernel
        kfree(message);
        return -EWOULDBLOCK;
    } else {
        attachMessageToChannel(message, channelID, length);
    }
    return i;
}

static long device_ioctl(struct file* file, unsigned int ioctl_command_id, unsigned long ioctl_param) { //function general build taken from Eran's code
    unsigned int channelID;
    if(MSG_SLOT_CHANNEL == ioctl_command_id ) { // handle error: different ioctl command, not supported
        channelID = (unsigned int)ioctl_param;
        if (channelID == 0) {
            return -EINVAL;
        }
        file->private_data = (void*)ioctl_param; // associating file descriptor with channel id
        regiserChannelIfNeeded(channelID);
    }
    else {
        return -EINVAL;
    }
    return SUCCESS;
}

struct file_operations Fops = { //from Eran`s code
        .owner	  = THIS_MODULE,
        .read           = device_read,
        .write          = device_write,
        .open           = device_open,
        .unlocked_ioctl = device_ioctl,
};

static __init int message_slot_init(void) {
    int isValidInit = register_chrdev(MAJOR_NUMBER, DEVICE_NAME, &Fops);
    if (isValidInit != 0)
        printk(KERN_ERR "Error in initialization\n" );
    return 0;
}

static void __exit message_slot_cleanup(void) {
    freeAllMinors(); // free data structures
    unregister_chrdev(MAJOR_NUMBER, DEVICE_NAME);
}

module_init(message_slot_init);
module_exit(message_slot_cleanup);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Shira Pasternak");
