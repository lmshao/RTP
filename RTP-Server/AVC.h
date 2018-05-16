//
// Created by Liming Shao on 2018/5/10.
//

#ifndef RTPSERVER_AVC_H
#define RTPSERVER_AVC_H

#include <stdint.h>

/* copy from FFmpeg libavformat/acv.c */
const uint8_t *ff_avc_find_startcode(const uint8_t *p, const uint8_t *end);

#endif //RTPSERVER_AVC_H
