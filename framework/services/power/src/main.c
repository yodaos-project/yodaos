#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>

#include <json-c/json.h>
#include <common.h>
#include <eloop_mini/eloop.h>
#include "rklog/RKLog.h"

static pthread_t p_pm_flora;
//extern void *pm_flora_handle(void *args);

int main(int argc, char *argv[argc]) {
    int ret = 0;

    ret = power_module_init();
    if (ret < 0) {
        pm_print("pms get power device error %d\n", ret);
        return -1;
    }
    pm_print("pms: power_module_init finished! \n");

    eloop_init();
    ret = pthread_create(&p_pm_flora, NULL, pm_flora_handle, NULL);
    if (ret != 0) {
        pm_print("can't create thread: %s\n", strerror(ret));
        return -2;
    }
    pm_print("pms: pm_flora_handle finished! \n");

    pthread_join(p_pm_flora, NULL);

    eloop_stop();
    eloop_exit();
    return 0;
}
