#include <linux/errno.h>

#define SUCCESS 0
#define BUFFER_LEN 128
#define NAME "message_slot"
#define MAJ_NUMBER 235
#define MSG_SLOT_CHANNEL _IOW(MAJ_NUMBER, 0, unsigned int)


typedef struct channel{
    char message[BUFFER_LEN];
    int messageSize;
    unsigned long channelId;
    struct channel *next;
} channel;

typedef struct file_data {
    unsigned int minorNum;
    channel *currentChannel;
    unsigned long currentChannelId;
} fileData;
