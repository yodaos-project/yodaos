#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <json-c/json.h>
#include <flora-cli.h>

static flora_cli_t cli;
static FILE *fp;

void print_time() {
    time_t now;
    struct tm *tm_now;
    struct timeval tv;

    time(&now);
    tm_now = localtime(&now);
    gettimeofday(&tv, NULL);

    printf("[%04d-%02d-%02d %02d:%02d:%02d %03d]  ", tm_now->tm_year+1900, tm_now->tm_mon+1, tm_now->tm_mday, tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec, tv.tv_usec/1000);
}

static void pm_service_command_callback(const char* name, uint32_t msgtype, caps_t msg, void* arg) {
    const char *buf = NULL;

    print_time();
    printf("name :: %s\n", name);

    caps_read_string(msg, &buf);

    print_time();
    printf("data :: %s\n", buf);

    if (fp)
        fprintf(fp, "battery_info :: %s\n", buf);
}

static void conn_disconnected(void* arg) {
    printf("disconnected\n");
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
            flora_cli_subscribe(cli, "battery.info");
            break;
        }
    }

    return 0;
}

int main(int argc, char *argv[argc]) {

    fp = fopen("/data/.bat_tmp", "wt+");
    if(NULL == fp)
	printf("open /data/.bat_tmp failed.\n");

    pm_flora_init();

    if (argc == 1) {
        while (1) {
            sleep(1);
        }
    } else {
    if (strcmp(argv[1], "get") == 0)
        printf("battery info store in /data/.bat_tmp\n");
    else
	printf("usage : \n"
		"bat_monitor : persistently monitor battery info\n"
		"bat_monitor get : get battery info once\n");
    }

    sleep(2);
    if (fp)
	fclose(fp);
    fp = NULL;

    return 0;
}
