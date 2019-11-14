#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <cutils/log.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <assert.h>
#include <sys/time.h>
#include <pthread.h>
#include <cutils/properties.h>
#include <hardware/autoconnect.h>

static autoconnect_device_t *au_dev = NULL;

void pkt_process(char *buf, int length)
{
    int i;
    printf("Receive data:\n");
    for(i=0;i<length;i++){
        printf("%d=%x",i,buf[i]);
    }
}

static int open_au_dev(void)
{
    struct hw_module_t* hw_mod;

    int ret = hw_get_module(AUTOCONNECT_HW_ID, (const struct hw_module_t **) &hw_mod);
    if(ret == 0){
        printf("id=%s\n", hw_mod->id);
        ret = hw_mod->methods->open(hw_mod, NULL, (struct hw_device_t **) &au_dev);
        if(ret){
            printf("%s open hw module fail: %s\n", __func__, AUTOCONNECT_HW_ID);
            return -1;
        }else{
            return 0;
        }
    }else{
        printf ("%s failed to fetch hw module: %s\n", __func__, AUTOCONNECT_HW_ID);
        return -1;
    }
}

int main_process()
{
    int nRes;
    int freq = 0;
    char bssid[128];
    char mac[128];
    char ssid[128];

    printf("%s\n", __func__);

    nRes = open_au_dev();
    if(nRes < 0){
        printf("open_au_dev err\n");
        return -1;
    }

    nRes = au_dev->dev_init();
    if(nRes < 0){
        printf("failed to dev_init\n");
        goto exit;
    }

    nRes = au_dev->recv_start(&pkt_process);
    if(nRes < 0){
        printf("failed to recv_start\n");
        goto exit;
    }

    sleep(5);

    nRes = au_dev->recv_stop();
    if(nRes < 0){
        printf("failed to recv_start\n");
        goto exit;
    }

    char actbuf[30] = "actiondata_test_data";
    nRes = au_dev->send_action(actbuf, sizeof(actbuf));
    if(nRes < 0){
        printf("failed to recv_start\n");
        goto exit;
    }

    nRes = au_dev->get_info(&freq, bssid, mac, ssid);
    if(nRes < 0){
        printf("failed to draw led_array_dev\n");
        goto exit;
    }else{
        printf("freq=%d,bssid=%s,mac=%s,ssid=%s\n", freq,bssid,mac,ssid);
    }

exit:
    nRes = au_dev->dev_deinit();
    if(nRes < 0){
        printf("failed to recv_start\n");
    }
    nRes = au_dev->dev_close(au_dev);
    if(nRes < 0){
        printf("failed to recv_start\n");
    }
    return 0;

}

static void show_help(void)
{
    fprintf(stdout, "usage: \n");
    fprintf(stdout, "<command> :help\n");
}

int main(int argc, char **argv)
{
    int ret = -1;

    if (argc > 1) {
        if (!strcmp(argv[1], "-h")) {
            show_help();
        }
    }else{
        main_process();
    }

    return 0;
}
