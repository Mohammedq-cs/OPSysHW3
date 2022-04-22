#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "message_slot.h"

int main(int argc, char *argv[]){
    int fileDesc, tmpVal;
    char *file_path;
    unsigned long channelId;
    char buffer[BUFFER_LEN];
    if (argc != 3){
        fprintf(stderr, "Please enter 3 arguments:\n(1) message slot file path\n(2) target channel id\n");
        exit(1);
    }
    file_path = argv[1];
    channelId = atoi(argv[2]);
    fileDesc = open(file_path, O_RDWR);
    if (fileDesc < 0){
        fprintf(stderr, "opening device has failed, error: %s\n", strerror(errno));
        exit(1);
    }
    tmpVal = ioctl(fileDesc, MSG_SLOT_CHANNEL, channelId);
    if (tmpVal < 0){
        fprintf(stderr, "ioctl has failed, error: %s\n", strerror(errno));
        exit(1);
    }
    tmpVal = read(fileDesc, buffer, BUFFER_LEN);
    if (tmpVal <= 0){
        fprintf(stderr, "read has failed, error: %s\n", strerror(errno));
        exit(1);
    }
    /*here tmpVal is equal to the message len, that is why we give it to write*/
    tmpVal = write(1, buffer, tmpVal);
    if (tmpVal <= 0){
        fprintf(stderr, "printing to stdout has failed, error: %s\n", strerror(errno));
        exit(1);
    }
    tmpVal = close(fileDesc);
    if (tmpVal < 0){
        fprintf(stderr, "closing device file has failed, error: %s\n", strerror(errno));
        exit(1);
    }
    exit(0);
}
