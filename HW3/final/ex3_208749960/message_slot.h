#ifndef HW3_MESSAGE_SLOT_H
#define HW3_MESSAGE_SLOT_H

#include <linux/ioctl.h>

#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUMBER, 0, unsigned int)

#define MAJOR_NUMBER 235
#define BUF_LEN 128
#define SUCCESS 0

#endif //HW3_MESSAGE_SLOT_H



