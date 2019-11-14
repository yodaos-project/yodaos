#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <sys/syslog.h>
#include <common.h>

static pthread_t p_bat_flora;
static pthread_t p_chg_flora;

int main(int argc, char *argv[argc]) {
    int ret = 0;

    ret = mHealthdConfigInit();
    if (ret != 0) {
        KLOG_ERROR(LOG_TAG "can't allocate memory\n");
        return -2;
    }

    mHealthdConfigUpdate(mHealthdConfig);

    ret = pthread_create(&p_bat_flora, NULL, bat_flora_handle, NULL);
    if (ret != 0) {
        KLOG_ERROR(LOG_TAG "can't create thread: %s\n", strerror(ret));
        return -2;
    }
    KLOG_INFO(LOG_TAG "bat_flora_handle finished! \n");

    if(mBootModeUpdate() == BOOT_MODE_CHARGING) {
	ret = pthread_create(&p_chg_flora, NULL, mPowerOffChargingPoll, NULL);
	if (ret != 0) {
            KLOG_ERROR(LOG_TAG "can't create thread: %s\n", strerror(ret));
	    return -2;
	}
	KLOG_INFO(LOG_TAG "mPowerOffChargingPoll finished! \n");
	pthread_join(p_chg_flora, NULL);
    }

    pthread_join(p_bat_flora, NULL);
    return 0;
}
