//
// Created by xiang on 2024/2/9.
//

#ifndef SCRCPY_MSG_H
#define SCRCPY_MSG_H

#include <libavformat/avformat.h>

int msg_pub_frame(AVFrame *frame);
int msg_sub_ctrl(void);

#endif //SCRCPY_MSG_H
