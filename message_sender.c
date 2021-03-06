#include <fcntl.h>
#include <unistd.h>    
#include <sys/ioctl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "message_slot.h"

int main(int argc, char** argv)
{
    char *file_path;
    unsigned long channelId;
    char* message;
    int fd;
    int ret;

    if (argc != 4)
    {
       printf("Enter 3 arguments:\n(1) message slot file path\n(2) target channel id\n(3) the message\n");
       return 0;
    }

    file_path = argv[1];
    channelId = atoi(argv[2]);
    message = argv[3];
    fd = open(file_path, O_RDWR);
    if (fd < 0){
        fprintf(stderr, "opening device file has failed, error: %s\n", strerror(errno));
        exit(1);
    }
    ret = ioctl(fd, MSG_SLOT_CHANNEL, channelId);
    if (ret < 0){
        fprintf(stderr, "ioctl has failed, error: %s\n", strerror(errno));
        exit(1);
    }
    ret = write(fd, message, strlen(message));
    if (ret < 0){
        fprintf(stderr, "writing to device has failed, error: %s\n", strerror(errno));
        exit(1);
    }
    ret = close(fd);
    if (ret < 0){
        fprintf(stderr, "closing device file has failed, error: %s", strerror(errno));
        exit(1);
    }
    exit(0);
}