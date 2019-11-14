/*
 * =====================================================================================
 *
 *       Filename:  key_hw.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  19/09/2019 10:15:05 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  bin.zhu (rokid),
 *   Organization:
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#if 0
const char* publicKey = "-----BEGIN PUBLIC KEY-----\n"\
"MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQC6bP195Mg6DShLGal4SoOE2ETE\n"\
"Xja10DSHLc7IgWcXy84ESXI0WLv42EGLd6ObF2K1LHTygewBgcl7ll+oHcu+2A0p\n"\
"FrEkGKTyeMYeD0JDHkTl5UL+1LX2EQgoGw5jse4XixaRIj1OLlgRn8QvjfMyZ32f\n"\
"K8I8JXgaw8ETSZsS/wIDAQAB\n"\
"-----END PUBLIC KEY-----\n";
#endif

int get_hardware_key(char *key_buf)
{
    int ret = 0;
    //strcpy(key_buf, publicKey);
    //ret = strlen(publicKey);
    return ret;
}
