//
// Created by Liming Shao on 2018/5/10.
//

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "RTPEnc.h"
#include "Utils.h"
#include "AVC.h"
#include "Network.h"

#define RTP_VERSION 2
#define RTP_H264    96

static UDPContext *gUdpContext;


int initRTPMuxContext(RTPMuxContext *ctx){
    ctx->seq = 0;
    ctx->timestamp = 0;
    ctx->ssrc = 0x12345678; // random number
    ctx->aggregation = 1;   // use Aggregation Unit
    ctx->buf_ptr = ctx->buf;
    ctx->payload_type = 0;  // 0, H.264/AVC; 1, HEVC/H.265
    return 0;
}

// enc RTP packet
void rtpSendData(RTPMuxContext *ctx, const uint8_t *buf, int len, int mark)
{
    int res = 0;

    /* build the RTP header */
    /*
     *
     *    0                   1                   2                   3
     *    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     *   |V=2|P|X|  CC   |M|     PT      |       sequence number         |
     *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     *   |                           timestamp                           |
     *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     *   |           synchronization source (SSRC) identifier            |
     *   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
     *   |            contributing source (CSRC) identifiers             |
     *   :                             ....                              :
     *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     *
     **/

    uint8_t *pos = ctx->cache;
    pos[0] = (RTP_VERSION << 6) & 0xff;      // V P X CC
    pos[1] = (uint8_t)((RTP_H264 & 0x7f) | ((mark & 0x01) << 7)); // M PayloadType
    Load16(&pos[2], (uint16_t)ctx->seq);    // Sequence number
    Load32(&pos[4], ctx->timestamp);
    Load32(&pos[8], ctx->ssrc);

    /* copy av data */
    memcpy(&pos[12], buf, len);

    res = udpSend(gUdpContext, ctx->cache, (uint32_t)(len + 12));
    printf("\nrtpSendData cache [%d]: ", res);
    for (int i = 0; i < 20; ++i) {
        printf("%.2X ", ctx->cache[i]);
    }
    printf("\n");

    memset(ctx->cache, 0, RTP_PAYLOAD_MAX+10);

    ctx->buf_ptr = ctx->buf;  // restore buf_ptr

    ctx->seq = (ctx->seq + 1) & 0xffff;
}

// 拼接NAL头部 在 ctx->buff, 然后ff_rtp_send_data
static void rtpSendNAL(RTPMuxContext *ctx, const uint8_t *nal, int size, int last){
    printf("rtpSendNAL  len = %d M=%d\n", size, last);

    // Single NAL Packet or Aggregation Packets
    if (size <= RTP_PAYLOAD_MAX){

        // Aggregation Packets
        if (ctx->aggregation){
            /*
             *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
             *  |STAP-A NAL HDR | NALU 1 Size | NALU 1 HDR & Data | NALU 2 Size | NALU 2 HDR & Data | ... |
             *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
             *
             * */
            int buffered_size = (int)(ctx->buf_ptr - ctx->buf);  // size of data in ctx->buf
            uint8_t curNRI = (uint8_t)(nal[0] & 0x60);           // NAL NRI

            // The remaining space in ctx->buf is less than the required space
            if (buffered_size + 2 + size > RTP_PAYLOAD_MAX) {
                rtpSendData(ctx, ctx->buf, buffered_size, 0);
                buffered_size = 0;
            }

            /*
             *    STAP-A/AP NAL Header
             *     +---------------+
             *     |0|1|2|3|4|5|6|7|
             *     +-+-+-+-+-+-+-+-+
             *     |F|NRI|  Type   |
             *     +---------------+
             * */
            if (buffered_size == 0){
                *ctx->buf_ptr++ = (uint8_t)(24 | curNRI);  // 0x18
            } else {
                uint8_t lastNRI = (uint8_t)(ctx->buf[0] & 0x60);
                if (curNRI > lastNRI){  // if curNRI > lastNRI, use new curNRI
                    ctx->buf[0] = (uint8_t)((ctx->buf[0] & 0x9F) | curNRI);
                }
            }

            // set STAP-A/AP NAL Header F = 1, if this NAL F is 1.
            ctx->buf[0] |= (nal[0] & 0x80);

            // NALU Size + NALU Header + NALU Data
            Load16(ctx->buf_ptr, (uint16_t)size);   // NAL size
            ctx->buf_ptr += 2;
            memcpy(ctx->buf_ptr, nal, size);        // NALU Header & Data
            ctx->buf_ptr += size;

            // meet last NAL, send all buf
            if (last == 1){
                rtpSendData(ctx, ctx->buf, (int)(ctx->buf_ptr - ctx->buf), 1);
            }
        }
        // Single NAL Unit RTP Packet
        else {
            /*
             *   0 1 2 3 4 5 6 7 8 9
             *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
             *  |F|NRI|  Type   | a single NAL unit ... |
             *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
             * */
            rtpSendData(ctx, nal, size, last);
        }

    } else {  // 分片分组
        /*
         *
         *  0                   1                   2
         *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3
         * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         * | FU indicator  |   FU header   |   FU payload   ...  |
         * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         *
         * */
        if (ctx->buf_ptr > ctx->buf){
            rtpSendData(ctx, ctx->buf, (int)(ctx->buf_ptr - ctx->buf), 0);
        }

        int headerSize;
        uint8_t *buff = ctx->buf;
        uint8_t type = nal[0] & 0x1F;
        uint8_t nri = nal[0] & 0x60;

        /*
         *     FU Indicator
         *    0 1 2 3 4 5 6 7
         *   +-+-+-+-+-+-+-+-+
         *   |F|NRI|  Type   |
         *   +---------------+
         * */
        buff[0] = 28;   // FU Indicator; FU-A Type = 28
        buff[0] |= nri;

        /*
         *      FU Header
         *    0 1 2 3 4 5 6 7
         *   +-+-+-+-+-+-+-+-+
         *   |S|E|R|  Type   |
         *   +---------------+
         * */
        buff[1] = type;     // FU Header uses NALU Header
        buff[1] |= 1 << 7;  // S(tart) = 1
        headerSize = 2;
        size -= 1;
        nal += 1;

        while (size + headerSize > RTP_PAYLOAD_MAX) {
            memcpy(&buff[headerSize], nal, (size_t)(RTP_PAYLOAD_MAX - headerSize));
            rtpSendData(ctx, buff, RTP_PAYLOAD_MAX, 0);
            nal += RTP_PAYLOAD_MAX - headerSize;
            size -= RTP_PAYLOAD_MAX - headerSize;
            buff[1] &= 0x7f;  // buff[1] & 0111111, S(tart) = 0
        }
        buff[1] |= 0x40;      // buff[1] | 01000000, E(nd) = 1
        memcpy(&buff[headerSize], nal, size);
        rtpSendData(ctx, buff, size + headerSize, last);
    }
}

// 从一段H264流中，查询完整的NAL发送，直到发送完此流中的所有NAL
void rtpSendH264HEVC(RTPMuxContext *ctx, UDPContext *udp, const uint8_t *buf, int size){
    const uint8_t *r;
    const uint8_t *end = buf + size;
    gUdpContext = udp;

    printf("\nrtpSendH264HEVC start\n");

    if (NULL == ctx || NULL == udp || NULL == buf ||  size <= 0){
        printf("rtpSendH264HEVC param error.\n");
        return;
    }

    r = ff_avc_find_startcode(buf, end);
    while (r < end){
        const uint8_t *r1;
        while (!*(r++));  // skip current startcode

        r1 = ff_avc_find_startcode(r, end);  // find next startcode

        // send a NALU (except NALU startcode), r1==end indicates this is the last NALU
        rtpSendNAL(ctx, r, (int)(r1-r), r1==end);

        // control transmission speed
        usleep(1000000/25);
        // suppose the frame rate is 25 fps
        ctx->timestamp += (90000.0/25);
        r = r1;
    }
}
