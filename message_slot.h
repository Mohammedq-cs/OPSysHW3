#define _MESSAGE_SLOT_H_

#include <linux/ioctl.h>

#define BUFFER_LEN 128
#define SUCCESS 0
#define MAJOR_NUM 235
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned int)
#define DEVICE_RANGE_NAME "message_slot"

typedef struct channel{
    char message[BUFFER_LEN];
    int message_size;
    unsigned long channelId;
    struct channel *next;
} channel;

typedef struct file_data {
    unsigned int minorNum;
    channel *currentChannel;
    unsigned long currentChannelId;
} fileData;
