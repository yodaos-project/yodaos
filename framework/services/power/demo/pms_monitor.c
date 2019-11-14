#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <json-c/json.h>
#include <flora-cli.h>
#include "rklog/RKLog.h"

static flora_cli_t cli;

void print_time() {
    time_t now;
    struct tm *tm_now;
    struct timeval tv;

    time(&now);
    tm_now = localtime(&now);
    gettimeofday(&tv, NULL);

    RKLog("[%04d-%02d-%02d %02d:%02d:%02d %03d]  ", tm_now->tm_year+1900, tm_now->tm_mon+1, tm_now->tm_mday, tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec, tv.tv_usec/1000);
}

static void pm_service_command_callback(const char* name, uint32_t msgtype, caps_t msg, void* arg) {
    const char *buf = NULL;

    print_time();
    RKLog("name :: %s\n", name);

    caps_read_string(msg, &buf);

    print_time();
    RKLog("data :: %s\n", buf);
}

static void conn_disconnected(void* arg) {
    RKLog("disconnected\n");
}

int pm_flora_init() {
    flora_cli_callback_t cb;

    memset(&cb, 0, sizeof(cb));

    cb.recv_post = pm_service_command_callback;
    //cb.recv_get = NULL;
    cb.disconnected = conn_disconnected;

    while (1) {
        if (flora_cli_connect("unix:/var/run/flora.sock", &cb, NULL, 0, &cli) != FLORA_CLI_SUCCESS) {
            sleep(1);
        } else {
            flora_cli_subscribe(cli, "power.sleep");
	    flora_cli_subscribe(cli, "power.force_sleep");
            flora_cli_subscribe(cli, "power.wakelock");
            flora_cli_subscribe(cli, "rtc.alarm");
	    flora_cli_subscribe(cli, "battery.info");
	    flora_cli_subscribe(cli, "rokid.turen.voice_coming");
	    flora_cli_subscribe(cli, "rokid.turen.sleep");
	    flora_cli_subscribe(cli, "rokid.turen.end_voice");
	    flora_cli_subscribe(cli, "yodart.vui._private_visibility");
            break;
        }
    }

    return 0;
}

int main(int argc, char *argv[argc]) {

    pm_flora_init();
    while (1) {
        sleep(1);
    }

    return 0;
}
