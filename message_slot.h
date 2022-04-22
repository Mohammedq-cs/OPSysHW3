#include <linux/errno.h>
#define SUCCESS 0
#define BUFFER_LEN 128
#define NAME "message_slot"
#define MAJ_NUMBER 235
#define MSG_SLOT_CHANNEL _IOW(MAJ_NUMBER, 0, unsigned int)


typedef struct channel{
    char message[BUFFER_LEN];
    int message_size;
    unsigned long channel_id;
    struct channel *next;
} channel;

typedef struct file_data {
    unsigned int minor_number;
    channel *current_channel;
    unsigned long current_channel_id;
} fdata;
