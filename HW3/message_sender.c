//
// Created by shira on 16/12/2022.
//


#include "message_slot.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

int main(int argc, char *argv[]){
    int file_desc;
    unsigned int channelID;
    char *path, *message;
    if (argc != 4) {
        perror("incorrect number of inputs\n");
        exit(1);
    }
    else {
        path = argv[1];
        sscanf(argv[2], "%d", &channelID);
        message = argv[3];
    }

    file_desc = open(path, O_RDWR);
    if(file_desc < 0) {
        perror("Can't open device file\n");
        exit(1);
    }

    if (ioctl( file_desc, MSG_SLOT_CHANNEL, channelID) < -1) {
        perror("failed setting channel\n");
        exit(1);
    }

    if (write(file_desc, message, 128) < -1) {
        perror("failed writing message\n");
        exit(1);
    }
    close(file_desc);
    return 0;
}
