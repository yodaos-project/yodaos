#include <stdio.h>
#include <string.h>
#include <json-c/json.h>
#include <flora-cli.h>
#include "rklog/RKLog.h"

static flora_cli_t cli;

static int app_get_choice(const char *querystring)
{
    int neg, value, c, base;
    int count = 0;

    base = 10;
    neg = 1;
    RKLog("%s => ", querystring);
    value = 0;
    do {
        c = getchar();

        if ((count == 0) && (c == '\n')) {
            return -1;
        }
        count ++;

        if ((c >= '0') && (c <= '9')) {
            value = (value * base) + (c - '0');
        } else if ((c >= 'a') && (c <= 'f')) {
            value = (value * base) + (c - 'a' + 10);
        } else if ((c >= 'A') && (c <= 'F')) {
            value = (value * base) + (c - 'A' + 10);
        } else if (c == '-') {
            neg *= -1;
        } else if (c == 'x') {
            base = 16;
        }

    } while ((c != EOF) && (c != '\n'));

    return value * neg;
}

static int pm_alarm_operation()
{
    int ret = 0;
    int choice;
    json_object *root = json_object_new_object();
    char *name = "Rokid-Alarm-Test";
    char *proto = "ROKID_ALARM";
    char *command = NULL;
    char *rtc_command = NULL;

    do {
        RKLog("rtc alarm test :\n");
        RKLog("    1 alarm on\n");
        RKLog("    2 alarm off\n");

        RKLog("    99 exit\n");

        choice = app_get_choice("Select cmd");

        switch(choice) {
        case 1:
            command = "ON";
            json_object_object_add(root, "name", json_object_new_string(name));
            json_object_object_add(root, "command", json_object_new_string(command));
            break;
        case 2:
            command = "OFF";
            json_object_object_add(root, "name", json_object_new_string(name));
            json_object_object_add(root, "command", json_object_new_string(command));
            break;
        case 99:
            break;
        default:
            break;
        }
        json_object_object_add(root, "proto", json_object_new_string(proto));

        rtc_command = json_object_to_json_string(root);

        caps_t msg = caps_create();
        caps_write_string(msg, (const char *)rtc_command);

        flora_cli_post(cli, "rtc.alarm", msg, FLORA_MSGTYPE_INSTANT);
        caps_destroy(msg);

    } while (choice != 99);

    json_object_put(root);

    return ret;
}

int pm_sleep_operation()
{
    int ret = 0;
    int choice = 0;
    const json_object *root = json_object_new_object();
    char *name = "Rokid-Sleep-Test";
    char *proto = "ROKID_SLEEP";
    char *command = NULL;
    char *pm_command = NULL;

    do {
        RKLog("power.sleep setting:\n");
        RKLog("    1 autosleep on\n");
        RKLog("    2 autosleep off\n");

        RKLog("    99 exit \n");

        choice = app_get_choice("Select cmd");

        json_object *root = json_object_new_object();

        switch(choice) {
        case 1:
            command = "ON";
            json_object_object_add(root, "name", json_object_new_string(name));
            json_object_object_add(root, "sleep", json_object_new_string(command));
            break;
        case 2:
            command = "OFF";
            json_object_object_add(root, "name", json_object_new_string(name));
            json_object_object_add(root, "sleep", json_object_new_string(command));
            break;
        case 99:
            break;
        default:
            break;
        }

        json_object_object_add(root, "proto", json_object_new_string(proto));

        pm_command = json_object_to_json_string(root);

        caps_t msg = caps_create();
        caps_write_string(msg, (const char *)pm_command);

        flora_cli_post(cli, "power.sleep", msg, FLORA_MSGTYPE_INSTANT);
        caps_destroy(msg);

    } while (choice != 99);

    json_object_put(root);

    return 0;
}

int pm_force_sleep_operation()
{
    int ret = 0;
    int choice = 0;
    const json_object *root = json_object_new_object();
    char *name = "Rokid-Force-Sleep-Test";
    char *proto = "ROKID_FORCE_SLEEP";
    char *command = NULL;
    char *pm_command = NULL;

    do {
        RKLog("power.force_sleep setting:\n");
        RKLog("    1 forcesleep on\n");

        RKLog("    99 exit \n");

        choice = app_get_choice("Select cmd");

        json_object *root = json_object_new_object();

        switch(choice) {
        case 1:
            command = "ON";
            json_object_object_add(root, "name", json_object_new_string(name));
            json_object_object_add(root, "sleep", json_object_new_string(command));
            break;
        case 99:
            break;
        default:
            break;
        }

        json_object_object_add(root, "proto", json_object_new_string(proto));

        pm_command = json_object_to_json_string(root);

        caps_t msg = caps_create();
        caps_write_string(msg, (const char *)pm_command);

        flora_cli_post(cli, "power.force_sleep", msg, FLORA_MSGTYPE_INSTANT);
        caps_destroy(msg);

    } while (choice != 99);

    json_object_put(root);

    return 0;
}

int pm_wakelock_operation()
{
    int choice = 0;
    json_object *root = json_object_new_object();
    char *proto = "ROKID_WAKELOCK";
    char *command = NULL;
    char *pm_command = NULL;

    do {
        RKLog("power.wakelock setting:\n");
        RKLog("    1 pms_wakelock request Rokid-Wakelock-Test1\n");
        RKLog("    2 pms_wakelock release Rokid-Wakelock-Test1\n");
        RKLog("    3 pms_wakelock request Rokid-Wakelock-Test2\n");
        RKLog("    4 pms_wakelock release Rokid-Wakelock-Test2\n");
        RKLog("    5 pms_wakelock request Rokid-Wakelock-Test3\n");
        RKLog("    6 pms_wakelock release Rokid-Wakelock-Test3\n");

        RKLog("    99 exit \n");

        choice = app_get_choice("Select cmd");

        json_object *root = json_object_new_object();

        switch(choice) {
        case 1:
            command = "ON";
            json_object_object_add(root, "name", json_object_new_string("Rokid-Wakelock-Test1"));
            json_object_object_add(root, "wakelock", json_object_new_string(command));
            break;
        case 2:
            command = "OFF";
            json_object_object_add(root, "name", json_object_new_string("Rokid-Wakelock-Test1"));
            json_object_object_add(root, "wakelock", json_object_new_string(command));
            break;
        case 3:
            command = "ON";
            json_object_object_add(root, "name", json_object_new_string("Rokid-Wakelock-Test2"));
            json_object_object_add(root, "wakelock", json_object_new_string(command));
            break;
        case 4:
            command = "OFF";
            json_object_object_add(root, "name", json_object_new_string("Rokid-Wakelock-Test2"));
            json_object_object_add(root, "wakelock", json_object_new_string(command));
            break;
        case 5:
            command = "ON";
            json_object_object_add(root, "name", json_object_new_string("Rokid-Wakelock-Test3"));
            json_object_object_add(root, "wakelock", json_object_new_string(command));
            break;
        case 6:
            command = "OFF";
            json_object_object_add(root, "name", json_object_new_string("Rokid-Wakelock-Test3"));
            json_object_object_add(root, "wakelock", json_object_new_string(command));
            break;
        case 99:
            break;
        default:
            break;
        }

        json_object_object_add(root, "proto", json_object_new_string(proto));

        pm_command = json_object_to_json_string(root);

        caps_t msg = caps_create();
        caps_write_string(msg, (const char *)pm_command);

        flora_cli_post(cli, "power.wakelock", msg, FLORA_MSGTYPE_INSTANT);
        caps_destroy(msg);

    } while (choice != 99);

    json_object_put(root);

    return 0;
}

int main_loop()
{
    int choice = 0;

    do {
        RKLog("PowerManger Service test cmd :\n");
        RKLog("    1 sleep\n");
        RKLog("    2 wakelock\n");
        RKLog("    3 alarm\n");
	RKLog("    4 force_sleep\n");

        RKLog("    99 exit\n");

        choice = app_get_choice("Select cmd");

        switch(choice) {
        case 1:
            pm_sleep_operation();
            break;
        case 2:
            pm_wakelock_operation();
            break;
        case 3:
            pm_alarm_operation();
            break;
        case 4:
            pm_force_sleep_operation();
	    break;
        case 99:
            break;
        default:
            break;
        }
    } while (choice != 99);

    return 0;
}

static void conn_disconnected(void* arg)
{
    RKLog("disconnected\n");
}

int pm_flora_init()
{

    while (1) {
        if (flora_cli_connect("unix:/var/run/flora.sock", NULL, NULL, 0, &cli) != FLORA_CLI_SUCCESS) {
            sleep(1);
        } else {
            flora_cli_subscribe(cli, "power.sleep");
	    flora_cli_subscribe(cli, "power.force_sleep");
            flora_cli_subscribe(cli, "power.wakelock");
            flora_cli_subscribe(cli, "rtc.alarm");
            break;
        }
    }

    return 0;
}

int main(int argc, char *argv[argc])
{

    pm_flora_init();
    main_loop();

    return 0;
}
