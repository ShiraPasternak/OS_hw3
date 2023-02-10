//
// Created by shira on 16/12/2022.
//


#include "message_slot.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

int main(int argc,char **argv){
    int file_desc;
    unsigned int channelID = 0;
    char *path, *stdout_buff;
    if (argc != 3) {
        perror("incorrect number of inputs\n");
        exit(1);
    } else {
        path = argv[1];
        sscanf(argv[2], "%d", &channelID);
    }
    file_desc = open(path, O_RDWR);
    if(file_desc < 0) {
        perror("Can't open device file\n");
        exit(1);
    }

    if (ioctl( file_desc, MSG_SLOT_CHANNEL, channelID) < -1) {
        perror("failed setting channel\n");
        close(file_desc);
        exit(1);
    }

    stdout_buff = (char*) calloc(BUF_LEN, sizeof(char));
    if (read(file_desc, stdout_buff, BUF_LEN) < -1) {
        perror("failed writing message to buffer\n");
        free(stdout_buff);
    	close(file_desc);
        exit(1);
    }

    if (write(STDOUT_FILENO, stdout_buff, strlen(stdout_buff)) < -1) { //https://stackoverflow.com/questions/19679978/write-the-contents-of-a-file-to-standard-out-using-system-calls
        perror("failed writing message to stdout\n");
        free(stdout_buff);
    	close(file_desc);
        exit(1);
    }

    free(stdout_buff);
    close(file_desc);
    return 0;
}
