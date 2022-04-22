#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "message_slot.h"

int main(int argc, char *argv[]){
    char *file_path;
    unsigned long channel_id;
    char* message;
    int fd;
    int ret;

    if (argc != 4){
        printf("Please enter 3 arguments:\n(1) message slot file path\n(2) target channel id\n(3) the message\n");
        return 0;
    }
    file_path = argv[1];
    channel_id = atoi(argv[2]);
    message = argv[3];
    fd = open(file_path, O_RDWR);
    if (fd < 0){
        fprintf(stderr, "error opening device file, error: %s\n", strerror(errno));
        exit(1);
    }
    ret = ioctl(fd, MSG_SLOT_CHANNEL, channel_id);
    if (ret < 0){
        fprintf(stderr, "ioctl has failed, error: %s\n", strerror(errno));
        exit(1);
    }
    ret = write(fd, message, strlen(message));
    if (ret < 0){
        fprintf(stderr, "error writing to device, error: %s\n", strerror(errno));
        exit(1);
    }
    ret = close(fd);
    if (ret < 0){
        fprintf(stderr, "error closing device file, error: %s", strerror(errno));
        exit(1);
    }
    exit(0);




}