#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/prctl.h>
#include <json-c/json.h>
#include <flora-cli.h>
#include <common.h>

static flora_cli_t cli;

static void bat_service_command_callback(const char* name, uint32_t msgtype, caps_t msg, void* arg)
{
#ifdef BAT_DEBUT
    const char *buf = NULL;

    printf("name :: %s\n", name);
    printf("data :: %s\n", buf);
    //Todo
#endif
}

int bat_service_msg_send(struct BatteryProperties* bp)
{
    int ret = 0;
    bool charging_on = false;
    json_object *root = json_object_new_object();
    //char *name = "battery_service";
    char *proto = "ROKID_BATTERY";
    const char *bat_command = NULL;

    if (mChargerNum) {
        charging_on = mBatteryPropertiesUpdate();
        mNeedSendMsg = mLastBatteryPropertiesUpdate(bp);
	mBatteryUseTimeReport(bp);
	mBatteryWriteChargerLeds(bp);
	mBatteryLowPowerCheck(bp);

	if (!mNeedSendMsg) {
	    json_object_put(root);
	    return 0;
	}

        if (charging_on)
	    json_object_object_add(root, "batChargingOnline", \
				   json_object_new_boolean(true));
        else
	    json_object_object_add(root, "batChargingOnline", \
				   json_object_new_boolean(false));

	json_object_object_add(root, "batSupported", \
			       json_object_new_boolean(true));
	json_object_object_add(root, "batPresent", \
			       json_object_new_boolean(true));
        json_object_object_add(root, "batLevel", \
			       json_object_new_int(bp->batteryLevel));
        json_object_object_add(root, "batVoltage", \
			       json_object_new_int(bp->batteryVoltage));
	json_object_object_add(root, "batChargingCurrent", \
			       json_object_new_int(bp->batteryChargingCurrent));
        json_object_object_add(root, "batTemp", \
			       json_object_new_int(bp->batteryTemperature / 10));
        json_object_object_add(root, "batTimetoEmpty", \
			       json_object_new_int(bp->batteryTimetoEmpty));
        json_object_object_add(root, "batSleepTimetoEmpty", \
			       json_object_new_int(bp->batterySleepTimetoEmpty));
        json_object_object_add(root, "batTimetoFull", \
			       json_object_new_int(bp->batteryTimetoFull));

    } else {
	if (!mNeedSendMsg) {
            json_object_put(root);
	    return 0;
	}
	json_object_object_add(root, "batSupported", \
			       json_object_new_boolean(false));
    }

    json_object_object_add(root, "proto", json_object_new_string(proto));

    bat_command = json_object_to_json_string(root);

    caps_t msg = caps_create();
    caps_write_string(msg, (const char *)bat_command);

    flora_cli_post(cli, "battery.info", msg, FLORA_MSGTYPE_PERSIST);
    caps_destroy(msg);

    json_object_put(root);
    mNeedSendMsg = false;
    return ret;

}

int bat_service_loop_run()
{
    while(1){
        bat_service_msg_send(&props);
        sleep(1);
    }

    free(mHealthdConfig);
    return 0;
}

static void conn_disconnected(void* arg)
{
    printf("disconnected\n");
}

void *bat_flora_handle(void *arg)
{
    flora_cli_callback_t cb;
    prctl(PR_SET_NAME, "battery_service_by_flora");

    memset(&cb, 0, sizeof(cb));

    cb.recv_post = bat_service_command_callback;
    //cb.recv_get = NULL;
    cb.disconnected = conn_disconnected;

    while (1) {
        if (flora_cli_connect("unix:/var/run/flora.sock", &cb, NULL, 0, &cli) != FLORA_CLI_SUCCESS) {
            sleep(1);
        } else {
            flora_cli_subscribe(cli, MSG_BAT);
            break;
        }
    }
    bat_service_loop_run();

    return NULL;
}

