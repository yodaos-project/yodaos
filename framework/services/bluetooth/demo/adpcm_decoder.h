#ifndef __AdpcmDecoder_h__
#define __AdpcmDecoder_h__

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef int (*adpcm_decode_cb)(void *caller_handle, uint8_t *buffer, int size);

void *adpcm_decoder_create(int sample_rate, int channels, int bit);

int adpcm_decode_frame(void *pParam, unsigned char *pData, int nLen,  adpcm_decode_cb completion, void *caller_handle);

void adpcm_decode_close(void *pParam);

#endif
