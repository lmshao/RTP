//
// Created by Liming Shao on 2018/5/10.
//

#ifndef RTPSERVER_RTPENC_H
#define RTPSERVER_RTPENC_H

#include "Network.h"

#define RTP_PAYLOAD_MAX     1400

typedef struct {
    uint8_t cache[RTP_PAYLOAD_MAX+12];  //RTP packet = RTP header + buf
    uint8_t buf[RTP_PAYLOAD_MAX];       // NAL header + NAL
    uint8_t *buf_ptr;

    int aggregation;   // 0: Single Unit, 1: Aggregation Unit
    int payload_type;  // 0, H.264/AVC; 1, HEVC/H.265
    uint32_t ssrc;
    uint32_t seq;
    uint32_t timestamp;
}RTPMuxContext;

int initRTPMuxContext(RTPMuxContext *ctx);

/* send a H.264/HEVC video stream */
void rtpSendH264HEVC(RTPMuxContext *ctx, UDPContext *udp, const uint8_t *buf, int size);

#endif //RTPSERVER_RTPENC_H
