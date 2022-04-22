#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "message_slot.h"

int main(int argc, char *argv[]){
    int fd, ret, i;
    char term = '\0';
    char *file_path;
    unsigned long channel_id;
    char buffer[BUFFER_LEN];
    if (argc != 3){
        fprintf(stderr, "Please enter 3 arguments:\n(1) message slot file path\n(2) target channel id");
        exit(1);
    }
    file_path = argv[1];
    channel_id = atoi(argv[2]);
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
    ret = read(fd, buffer, BUFFER_LEN);
    if (ret < 0){
        fprintf(stderr, "read has failed, error: %s\n", strerror(errno));
        exit(1);
    }
    ret = close(fd);
    if (ret < 0){
        fprintf(stderr, "closing device file has failed, error: %s\n", strerror(errno));
        exit(1);
    }
    for (i = 0; i < BUFFER_LEN; ++i){
    	if (strcmp(&buffer[i], &term) == 0){
    		break;
    	}
    }
    ret = write(1, buffer, i);
    if (ret < 0){
        fprintf(stderr, "printing has failed, error: %s\n", strerror(errno));
        exit(1);
    }
    close(fd);
    exit(0);

}
