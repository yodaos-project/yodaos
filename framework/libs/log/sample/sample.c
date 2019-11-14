#define LOG_TAG "sample"
#include "RKLog.h"

int main(int argc, char *argv[]){
    for(int i=0;i<20;i++){
        RKLog("This is the %d log from RKLog\n",i);//macro API
        RKNotice("This is the %d log from RKNotice\n",i);
        RKInfo("This is the %d log from RKInfo\n",i);
        RKWarn("This is the %d log from RKWarn\n",i);
        RKError("This is the %d log from RKError\n",i);
        ALOGV("This is the %d log from ALOGV\n",i);
        ALOGD("This is the %d log from ALOGD\n",i);
        ALOGI("This is the %d log from ALOGI\n",i);
        ALOGW("This is the %d log from ALOGW\n",i);
        ALOGE("This is the %d log from ALOGE\n",i);
        jslog(1,NULL,0,LOG_TAG,"this is the %d log from jslog\n",i);//C function API
        sleep(3);
    }
    return 0;

}