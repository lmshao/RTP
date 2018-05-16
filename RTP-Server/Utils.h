//
// Created by Liming Shao on 2018/5/10.
//

#ifndef RTPSERVER_UTILS_H
#define RTPSERVER_UTILS_H

#include <stdint.h>

uint8_t* Load8(uint8_t *p, uint8_t x);

uint8_t* Load16(uint8_t *p, uint16_t x);

uint8_t* Load32(uint8_t *p, uint32_t x);

/* read a complete file */
int readFile(uint8_t **stream, int *len, const char *file);

void dumpHex(const uint8_t *ptr, int len);

#endif //RTPSERVER_UTILS_H
