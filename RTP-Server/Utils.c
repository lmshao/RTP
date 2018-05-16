//
// Created by Liming Shao on 2018/5/10.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "Utils.h"

uint8_t* Load8(uint8_t *p, uint8_t x) {
    *p = x;
    return p+1;
}

uint8_t* Load16(uint8_t *p, uint16_t x) {
    p = Load8(p, (uint8_t)(x >> 8));
    p = Load8(p, (uint8_t)x);
    return p;
}

uint8_t* Load32(uint8_t *p, uint32_t x) {
    p = Load16(p, (uint16_t)(x >> 16));
    p = Load16(p, (uint16_t)x);
    return p;
}

int readFile(uint8_t **stream, int *len, const char *file) {
    FILE *fp = NULL;
    long size = 0;
    uint8_t *buf;

    printf("readFile %s\n", file);
    fp = fopen(file, "r");
    if (!fp)
        return -1;

#if 0
    fseek(fp, 0L, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
#else
    struct stat info = {0};
    stat(file, &info);
    size = info.st_size;
#endif

    buf = (uint8_t *)(malloc(size * sizeof(uint8_t)));
    memset(buf, 0, (size_t)size);

    if (fread(buf, 1, size, fp) != size){
        printf("read err.\n");
        return -1;
    }

    fclose(fp);

    *stream = buf;
    *len = (int)size;

    printf("File Size = %d Bytes\n", *len);
    return 0;
}

void dumpHex(const uint8_t *ptr, int len) {
    printf("%p [%d]: ", (void*)ptr, len);
    for (int i = 0; i < len; ++i) {
        printf("%.2X ", ptr[i]);
    }
    printf("\n");
}
