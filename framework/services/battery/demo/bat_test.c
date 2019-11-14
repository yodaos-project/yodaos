#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/time.h>
#include <json-c/json.h>
#include <flora-cli.h>

static flora_cli_t cli;

static int app_get_choice(const char *querystring)
{
    int neg, value, c, base;
    int count = 0;

    base = 10;
    neg = 1;
    printf("%s => ", querystring);
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

static int bat_info_operation()
{
    int ret = 0;
    int choice;
    json_object *root = json_object_new_object();
    char *proto = "ROKID_Battery_Test";
    const char *bat_command = NULL;

    do {
        printf("bat info test :\n");
        printf("    1 normal battery info msg\n");
        printf("    2 high temperature info msg\n");
        printf("    3 low temperature info msg\n");
        printf("    4 battery info with charging\n");
        printf("    5 battery info without charging\n");
        printf("    6 battery info with charging full\n");
        printf("    7 low battery level alert\n");

        printf("    99 exit\n");

        choice = app_get_choice("Select cmd");

        switch(choice) {
        case 1:
	    json_object_object_add(root, "batSupported", json_object_new_boolean(true));
            json_object_object_add(root, "batChargingOnline", json_object_new_boolean(0));
            json_object_object_add(root, "batChargingCurrent", json_object_new_int(0));
            json_object_object_add(root, "batChargingVoltage", json_object_new_int(0));
            json_object_object_add(root, "batChargingCounter", json_object_new_int(10));
	    json_object_object_add(root, "batTimetoFull", json_object_new_int(-1));
            json_object_object_add(root, "batTimetoEmpty", json_object_new_int(415));
	    json_object_object_add(root, "batSleepTimetoEmpty", json_object_new_int(8000));
            json_object_object_add(root, "batStatus", json_object_new_int(0));
            json_object_object_add(root, "batPresent", json_object_new_boolean(1));
            json_object_object_add(root, "batLevel", json_object_new_int(80));
            json_object_object_add(root, "batVoltage", json_object_new_int(4100));
            json_object_object_add(root, "batTemp", json_object_new_int(35));
            break;
        case 2:
	    json_object_object_add(root, "batSupported", json_object_new_boolean(true));
            json_object_object_add(root, "batChargingOnline", json_object_new_boolean(0));
            json_object_object_add(root, "batChargingCurrent", json_object_new_int(0));
            json_object_object_add(root, "batChargingVoltage", json_object_new_int(0));
            json_object_object_add(root, "batChargingCounter", json_object_new_int(10));
	    json_object_object_add(root, "batTimetoFull", json_object_new_int(-1));
            json_object_object_add(root, "batTimetoEmpty", json_object_new_int(415));
	    json_object_object_add(root, "batSleepTimetoEmpty", json_object_new_int(8000));
            json_object_object_add(root, "batStatus", json_object_new_int(0));
            json_object_object_add(root, "batPresent", json_object_new_boolean(1));
            json_object_object_add(root, "batLevel", json_object_new_int(80));
            json_object_object_add(root, "batVoltage", json_object_new_int(4100));
            json_object_object_add(root, "batTemp", json_object_new_int(58));
            break;
        case 3:
	    json_object_object_add(root, "batSupported", json_object_new_boolean(true));
            json_object_object_add(root, "batChargingOnline", json_object_new_boolean(0));
            json_object_object_add(root, "batChargingCurrent", json_object_new_int(0));
            json_object_object_add(root, "batChargingVoltage", json_object_new_int(0));
            json_object_object_add(root, "batChargingCounter", json_object_new_int(10));
	    json_object_object_add(root, "batTimetoFull", json_object_new_int(-1));
            json_object_object_add(root, "batTimetoEmpty", json_object_new_int(415));
	    json_object_object_add(root, "batSleepTimetoEmpty", json_object_new_int(8000));
            json_object_object_add(root, "batStatus", json_object_new_int(0));
            json_object_object_add(root, "batPresent", json_object_new_boolean(1));
            json_object_object_add(root, "batLevel", json_object_new_int(80));
            json_object_object_add(root, "batVoltage", json_object_new_int(4100));
            json_object_object_add(root, "batTemp", json_object_new_int(-5));
            break;
        case 4:
	    json_object_object_add(root, "batSupported", json_object_new_boolean(true));
            json_object_object_add(root, "batChargingOnline", json_object_new_boolean(1));
            json_object_object_add(root, "batChargingCurrent", json_object_new_int(1500));
            json_object_object_add(root, "batChargingVoltage", json_object_new_int(4200));
            json_object_object_add(root, "batChargingCounter", json_object_new_int(10));
	    json_object_object_add(root, "batTimetoFull", json_object_new_int(80));
            json_object_object_add(root, "batTimetoEmpty", json_object_new_int(415));
	    json_object_object_add(root, "batSleepTimetoEmpty", json_object_new_int(8000));
            json_object_object_add(root, "batStatus", json_object_new_int(1));
            json_object_object_add(root, "batPresent", json_object_new_boolean(1));
            json_object_object_add(root, "batLevel", json_object_new_int(80));
            json_object_object_add(root, "batVoltage", json_object_new_int(4100));
            json_object_object_add(root, "batTemp", json_object_new_int(35));
            break;
        case 5:
	    json_object_object_add(root, "batSupported", json_object_new_boolean(true));
            json_object_object_add(root, "batChargingOnline", json_object_new_boolean(0));
            json_object_object_add(root, "batChargingCurrent", json_object_new_int(0));
            json_object_object_add(root, "batChargingVoltage", json_object_new_int(0));
            json_object_object_add(root, "batChargingCounter", json_object_new_int(10));
	    json_object_object_add(root, "batTimetoFull", json_object_new_int(-1));
            json_object_object_add(root, "batTimetoEmpty", json_object_new_int(415));
	    json_object_object_add(root, "batSleepTimetoEmpty", json_object_new_int(8000));
            json_object_object_add(root, "batStatus", json_object_new_int(0));
            json_object_object_add(root, "batPresent", json_object_new_boolean(1));
            json_object_object_add(root, "batLevel", json_object_new_int(80));
            json_object_object_add(root, "batVoltage", json_object_new_int(4100));
            json_object_object_add(root, "batTemp", json_object_new_int(35));
            break;
        case 6:
	    json_object_object_add(root, "batSupported", json_object_new_boolean(true));
            json_object_object_add(root, "batChargingOnline", json_object_new_boolean(0));
            json_object_object_add(root, "batChargingCurrent", json_object_new_int(0));
            json_object_object_add(root, "batChargingVoltage", json_object_new_int(0));
            json_object_object_add(root, "batChargingCounter", json_object_new_int(10));
	    json_object_object_add(root, "batTimetoFull", json_object_new_int(0));
            json_object_object_add(root, "batTimetoEmpty", json_object_new_int(520));
	    json_object_object_add(root, "batSleepTimetoEmpty", json_object_new_int(9000));
            json_object_object_add(root, "batStatus", json_object_new_int(0));
            json_object_object_add(root, "batPresent", json_object_new_boolean(1));
            json_object_object_add(root, "batLevel", json_object_new_int(100));
            json_object_object_add(root, "batVoltage", json_object_new_int(4200));
            json_object_object_add(root, "batTemp", json_object_new_int(35));
            break;
        case 7:
	    json_object_object_add(root, "batSupported", json_object_new_boolean(true));
            json_object_object_add(root, "batChargingOnline", json_object_new_boolean(0));
            json_object_object_add(root, "batChargingCurrent", json_object_new_int(0));
            json_object_object_add(root, "batChargingVoltage", json_object_new_int(0));
            json_object_object_add(root, "batChargingCounter", json_object_new_int(10));
	    json_object_object_add(root, "batTimetoFull", json_object_new_int(-1));
            json_object_object_add(root, "batTimetoEmpty", json_object_new_int(95));
	    json_object_object_add(root, "batSleepTimetoEmpty", json_object_new_int(2000));
            json_object_object_add(root, "batStatus", json_object_new_int(0));
            json_object_object_add(root, "batPresent", json_object_new_boolean(1));
            json_object_object_add(root, "batLevel", json_object_new_int(19));
            json_object_object_add(root, "batVoltage", json_object_new_int(3700));
            json_object_object_add(root, "batTemp", json_object_new_int(35));
            break;
        case 99:
            break;
        default:
            break;
        }
        json_object_object_add(root, "proto", json_object_new_string(proto));

        bat_command = json_object_to_json_string(root);

        caps_t msg = caps_create();
        caps_write_string(msg, (const char *)bat_command);

        flora_cli_post(cli, "battery.info", msg, FLORA_MSGTYPE_PERSIST);
        caps_destroy(msg);

    } while (choice != 99);

    json_object_put(root);

    return ret;
}

int main_loop()
{
    int choice = 0;

    do {
        printf("Battery Service test cmd :\n");
        printf("    1 battery.info\n");
        printf("    99 exit\n");

        choice = app_get_choice("Select cmd");

        switch(choice) {
        case 1:
            bat_info_operation();
            break;
        case 99:
            break;
        default:
            break;
        }
    } while (choice != 99);

    return 0;
}

static const char *option_string = "a:b:c:d:e:f:g:i:j:k:l:m:h?";
int bat_test(int argc, char *argv[argc])
{
    int opt;
    int val;
    json_object *root = json_object_new_object();
    char *proto = "ROKID_Battery_Test";
    const char *bat_command = NULL;

    while((opt = getopt(argc, argv, option_string)) != -1) {

	switch(opt) {
	case 'a':
	    val = atoi(optarg);
	    json_object_object_add(root, "batSupported", json_object_new_boolean(val));
	    break;
	case 'b':
	    val = atoi(optarg);
	    json_object_object_add(root, "batChargingOnline", json_object_new_boolean(val));
	    break;
	case 'c':
	    val = atoi(optarg);
	    json_object_object_add(root, "batChargingCurrent", json_object_new_int(val));
	    break;
	case 'd':
	    val = atoi(optarg);
	    json_object_object_add(root, "batChargingVoltage", json_object_new_int(val));
	    break;
	case 'e':
	    val = atoi(optarg);
	    json_object_object_add(root, "batTimetoFull", json_object_new_int(val));
	    break;
	case 'f':
	    val = atoi(optarg);
	    json_object_object_add(root, "batTimetoEmpty", json_object_new_int(val));
	    break;
	case 'g':
	    val = atoi(optarg);
	    json_object_object_add(root, "batSleepTimetoEmpty", json_object_new_int(val));
	    break;
	case '?':
	case 'h':
	    printf("bat_test	[-a batSupported(0/1)]	[-b batChargingOnline(0/1)]\n"
		"		[-c batChargingCur(mA)]	[-d batChargingVoltage(mv)]\n"
		"		[-e batTimetoFull(min)]	[-f batTimetoEmpty(min)]\n"
		"		[-g batSleepTime(min)]	[-i batStatus(int)]\n"
		"		[-j batPreset(0/1)]	[-k batLevel(0-100)]\n"
		"		[-l batVoltage(mv)]	[-m batTemp(C)]\n");
	    return 0;
	case 'i':
	    val = atoi(optarg);
	    json_object_object_add(root, "batStatus", json_object_new_int(val));
	    break;
	case 'j':
	    val = atoi(optarg);
	    json_object_object_add(root, "batPresent", json_object_new_boolean(val));
	    break;
	case 'k':
	    val = atoi(optarg);
	    json_object_object_add(root, "batLevel", json_object_new_int(val));
	    break;
	case 'l':
	    val = atoi(optarg);
	    json_object_object_add(root, "batVoltage", json_object_new_int(val));
	    break;
	case 'm':
	    val = atoi(optarg);
	    json_object_object_add(root, "batTemp", json_object_new_int(val));
	    break;
	default:
	    break;
	}
    }
    json_object_object_add(root, "proto", json_object_new_string(proto));

    bat_command = json_object_to_json_string(root);

    caps_t msg = caps_create();
    caps_write_string(msg, (const char *)bat_command);

    flora_cli_post(cli, "battery.info", msg, FLORA_MSGTYPE_PERSIST);

    caps_destroy(msg);
    json_object_put(root);

    return 0;
}
#if 0
static void conn_disconnected(void* arg)
{
    printf("disconnected\n");
}
#endif

int bat_flora_init()
{

    while (1) {
        if (flora_cli_connect("unix:/var/run/flora.sock", NULL, NULL, 0, &cli) != FLORA_CLI_SUCCESS) {
            sleep(1);
        } else {
            flora_cli_subscribe(cli, "battery.info");
            break;
        }
    }

    return 0;
}

int main(int argc, char *argv[argc])
{

    bat_flora_init();
    if (argc == 1)
        main_loop();
    else
	bat_test(argc, argv);

    return 0;
}
