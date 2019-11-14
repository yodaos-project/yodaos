#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/prctl.h>
#include <json-c/json.h>
#include <flora-cli.h>

#include <common.h>
#include <eloop_mini/eloop.h>
#include "rklog/RKLog.h"

struct wakelock_node {
	char* name;
	struct wakelock_node* next;
};

static flora_cli_t cli;
static int wakelock_count;
static int chg_flag = -1;
static struct wakelock_node* wakelock_list_head;

static void pm_wakelock_init()
{
    wakelock_count = 0;
    wakelock_list_head = NULL;
}

static int pm_wakelock_add(char* wakelock_name)
{
    struct wakelock_node* phead = wakelock_list_head;
    struct wakelock_node* ptemp;

    if(wakelock_name == NULL)
	return -1;
    ptemp = phead;
    while(phead){
	if(strcmp(phead->name, wakelock_name) == 0){
		return wakelock_count;
	}else{
		ptemp = phead;
		phead = phead->next;
	}
    }
    if(phead == NULL){
	phead=(struct wakelock_node*)malloc(sizeof(struct wakelock_node));
	memset(phead, 0, sizeof(struct wakelock_node));
	phead->name=(char*)malloc(strlen(wakelock_name)+1);
	strcpy(phead->name, wakelock_name);
	wakelock_count++;
	if(wakelock_list_head == NULL)
		wakelock_list_head = phead;
	if(ptemp)
		ptemp->next = phead;
    }
    return wakelock_count;
}

static int pm_wakelock_del(char* wakelock_name)
{
    struct wakelock_node* phead = wakelock_list_head;
    struct wakelock_node* ptemp;

    if(wakelock_name == NULL)
	return -1;
    ptemp = phead;
    while(phead){
	if(strcmp(phead->name, wakelock_name) == 0){
		free(phead->name);
		if(phead == wakelock_list_head)
			wakelock_list_head = phead->next;
		else
			ptemp->next = phead->next;
		free(phead);
		wakelock_count--;
		return wakelock_count;
	}else{
		ptemp = phead;
		phead = phead->next;
	}
    }
    return wakelock_count;
}

static int pm_wakelock_scan()
{
    struct wakelock_node* phead = wakelock_list_head;
    while(phead){
	RKLog("wakelock:%s,",phead->name);
	phead = phead->next;
    }
    RKLog("\n");
    return wakelock_count;
}

int pm_autosleep_command_handle(json_object *obj)
{
    char *command = NULL;
    json_object *pm_cmd = NULL;

    if (obj == NULL) {
	return -1;
    }

    if (json_object_object_get_ex(obj, "sleep", &pm_cmd) == TRUE) {
         command = (char *)json_object_get_string(pm_cmd);
         RKLog("pms :: sleep %s \n", command);

	 if (strcmp(command, "ON") == 0) {
	    json_object *p_name = NULL;
	    char *name = NULL;

            if (json_object_object_get_ex(obj, "name", &p_name) == TRUE) {
                name = (char *)json_object_get_string(p_name);
                RKLog("pms :: name %s \n", name);
            }
	    pm_wakelock_scan();
	    power_autosleep_request(SLEEP_ON); //sleep begin
	 } else {
            power_autosleep_request(SLEEP_OFF); //sleep disable
	 }
    }

    return 0;
}

int pm_sleep_command_handle(json_object *obj)
{
    char *command = NULL;
    json_object *pm_cmd = NULL;

    if (obj == NULL) {
        return -1;
    }

    if (json_object_object_get_ex(obj, "sleep", &pm_cmd) == TRUE) {
         command = (char *)json_object_get_string(pm_cmd);
         RKLog("pms :: sleep %s \n", command);

         if (strcmp(command, "ON") == 0) {
            json_object *p_name = NULL;
            char *name = NULL;

            if (json_object_object_get_ex(obj, "name", &p_name) == TRUE) {
                name = (char *)json_object_get_string(p_name);
                RKLog("pms :: name %s \n", name);
            }
            if(power_lock_state_poll()==NO_LOCK_STATE)
                power_sleep_request(SLEEP_ON); //sleep begin
         }
    }

    return 0;
}

int pm_wakelock_command_handle(json_object *obj)
{
    char *command = NULL;
    json_object *pm_cmd = NULL;

    if (obj == NULL) {
	return -1;
    }

    if (json_object_object_get_ex(obj, "wakelock", &pm_cmd) == TRUE) {
         command = (char *)json_object_get_string(pm_cmd);
         RKLog("pms :: wakelock %s \n", command);

	 if (strcmp(command, "ON") == 0) {
	    json_object *p_name = NULL;
	    char *name = NULL;

            if (json_object_object_get_ex(obj, "name", &p_name) == TRUE) {
                name = (char *)json_object_get_string(p_name);
                RKLog("pms :: name %s \n", name);
            }
            if(pm_wakelock_add(name) == 1)
		power_wakelock_request(PART_LOCK_STATE, PMS_WAKELOCK);
	 } else if (strcmp(command, "OFF") == 0) {
	    json_object *p_name = NULL;
	    char *name = NULL;

	    if (json_object_object_get_ex(obj, "name", &p_name) == TRUE) {
		name = (char *)json_object_get_string(p_name);
		RKLog("pms :: name %s \n", name);
	    }
            if(pm_wakelock_del(name) == 0)
		power_wakelock_release(PMS_WAKELOCK);
	 }
    }

    return 0;
}

int pm_battery_command_handle(json_object *obj)
{
    char *command = NULL;
    json_object *pm_cmd = NULL;

    if (obj == NULL)
	return -1;

    if (json_object_object_get_ex(obj, "batChargingOnline", &pm_cmd)) {
	command = (char *)json_object_get_string(pm_cmd);
	RKLog("pms :: batChargingOnline is %s\n", command);

	if ((strcmp(command, "true") == 0) && (chg_flag != 1)) {
            chg_flag = 1;
	    power_wakelock_request(PART_LOCK_STATE, CHG_WAKELOCK);
	}
	else if ((strcmp(command, "false") == 0) && (chg_flag == 1)) {
            chg_flag = 0;
	    power_wakelock_release(CHG_WAKELOCK);
        }
    }

    return 0;
}

#define IDLE_TIMEOUT_S	60

static int idle_status = 0;
static const char *appId = NULL;
static eloop_id_t e_power_idle_id;

static void power_idle_timeout_func(void *eloop_ctx)
{
     flora_call_result result = {0};
     int ret;
     caps_t msg;
     msg = caps_create();
     caps_write_string(msg, "deep-idle enter");
     RKLog("pms: idle timeout enter deep-idle state\n");
     ret = flora_cli_call(cli, MSG_DEEP_IDLE, msg, "display-service", &result, 1000);
     if(ret) {
         RKLog("pms: display-service handle deep-idle enter msg error(%d)\n", ret);
     }
     ret = flora_cli_call(cli, MSG_DEEP_IDLE, msg, "bluetoothservice-agent", &result, 1000);
     if(ret) {
         RKLog("pms: bluetoothservice-agent handle deep-idle enter msg error(%d)\n", ret);
     }
     ret = flora_cli_call(cli, MSG_DEEP_IDLE, msg, "net_manager", &result, 1000);
     if(ret) {
         RKLog("pms: net_manager handle deep-idle enter msg error(%d)\n", ret);
     }
     //Todo: flora_cli_call to other deep-idle related services.
     //...
     //flora_cli_call end
     caps_destroy(msg);

     power_deep_idle_enter();

     msg = caps_create();
     caps_write_string(msg, "deep-idle exit");
     ret = flora_cli_call(cli, MSG_DEEP_IDLE, msg, "display-service", &result, 1000);
     if(ret) {
         RKLog("pms: display-service handle deep-idle exit msg error(%d)\n", ret);
     }
     ret = flora_cli_call(cli, MSG_DEEP_IDLE, msg, "bluetoothservice-agent", &result, 1000);
     if(ret) {
         RKLog("pms: bluetoothservice-agent handle deep-idle exit msg error(%d)\n", ret);
     }
     ret = flora_cli_call(cli, MSG_DEEP_IDLE, msg, "net_manager", &result, 1000);
     if(ret) {
         RKLog("pms: net_manager handle deep-idle exit msg error(%d)\n", ret);
     }
     //Todo: flora_cli_call to other deep-idle related services.
     //...
     //flora_cli_call end
     caps_destroy(msg);
     flora_result_delete(&result);
}

static int pm_idle_entry(int flag)
{
    if (flag == IDLE_ENTER) {
        if(!idle_status) {
            power_idle_enter();
            idle_status = 1;
            eloop_timer_start(e_power_idle_id);
        }
    } else {
        if(idle_status) {
            power_idle_exit();
            idle_status = 0;
            eloop_timer_stop(e_power_idle_id);
        }
    }
    return 0;
}

static int pm_msg_map_int(const char* name)
{
    if (strcmp(name, MSG_SLEEP) == 0)
        return NUM_MSG_SLEEP;
    else if (strcmp(name, MSG_FORCE_SLEEP) == 0)
        return NUM_MSG_FORCE_SLEEP;
    else if (strcmp(name, MSG_LOCK) == 0)
        return NUM_MSG_LOCK;
    else if (strcmp(name, MSG_ALARM) == 0)
        return NUM_MSG_ALARM;
    else if (strcmp(name, MSG_BATTERY) == 0)
        return NUM_MSG_BATTERY;
    else if (strcmp(name, MSG_VOICE_COMMING) == 0)
        return NUM_MSG_VOICE_COMMING;
    else if (strcmp(name, MSG_VOICE_END) ==0)
        return NUM_MSG_VOICE_END;
    else if (strcmp(name, MSG_VOICE_SLEEP) == 0)
	return NUM_MSG_VOICE_SLEEP;
    else if (strcmp(name, MSG_SYS_STATUS) == 0)
	return NUM_MSG_SYS_STATUS;
    else
        return 0;
}

static void pm_service_command_callback(const char* name, uint32_t msgtype, caps_t msg, void* arg)
{
    const char *buf = NULL;
    json_object *pm_command = NULL;

    caps_read_string(msg, &buf);
    RKLog("name :: %s\n", name);
    RKLog("data :: %s\n", buf);

    switch (pm_msg_map_int(name)) {
    case NUM_MSG_SLEEP:
        pm_command = json_tokener_parse(buf);
        pm_autosleep_command_handle(pm_command);
        break;
    case NUM_MSG_FORCE_SLEEP:
        pm_command = json_tokener_parse(buf);
        pm_sleep_command_handle(pm_command);
        break;
    case NUM_MSG_LOCK:
        pm_command = json_tokener_parse(buf);
        pm_wakelock_command_handle(pm_command);
	break;
    case NUM_MSG_ALARM:
	//todo
	break;
    case NUM_MSG_BATTERY:
	pm_command = json_tokener_parse(buf);
	RKLog("pms: battery.info receive\n");
	pm_battery_command_handle(pm_command);
	break;
    case NUM_MSG_VOICE_COMMING:
	RKLog("pms: rokid.turen.voice_coming receive\n");
	//power_wakelock_request(PART_LOCK_STATE, VAD_WAKELOCK);
	pm_idle_entry(IDLE_EXIT);
	break;
    case NUM_MSG_VOICE_END:
	RKLog("pms: rokid.turen.end_voice receive\n");
	if(!appId)
		pm_idle_entry(IDLE_ENTER);
	break;
    case NUM_MSG_VOICE_SLEEP:
	RKLog("pms: rokid.turen.sleep receive\n");
	pm_wakelock_scan();
	//todo
	//power_wakelock_release(VAD_WAKELOCK);
	break;
    case NUM_MSG_SYS_STATUS:
	if(buf==NULL) {
            RKLog("pms: idle enter receive\n");
            pm_idle_entry(IDLE_ENTER);
	} else {
            RKLog("pms: idle exit receive\n");
            pm_idle_entry(IDLE_EXIT);
	}
	appId = buf;
	break;
    default:
	break;
    }

    json_object_put(pm_command);
}

int pm_service_loop_run()
{
    while(1){
        sleep(1);
    }
}
static void conn_disconnected(void* arg)
{
    RKLog("disconnected\n");
}

void *pm_flora_handle(void *arg)
{
    flora_cli_callback_t cb;
    prctl(PR_SET_NAME,"powermanager_service_by_flora");

    memset(&cb, 0, sizeof(cb));

    cb.recv_post = pm_service_command_callback;
    //cb.recv_get = NULL;
    cb.disconnected = conn_disconnected;

    while (1) {
        if (flora_cli_connect("unix:/var/run/flora.sock", &cb, NULL, 0, &cli) != FLORA_CLI_SUCCESS) {
            sleep(1);
        } else {
            flora_cli_subscribe(cli, MSG_SLEEP);
            flora_cli_subscribe(cli, MSG_FORCE_SLEEP);
            flora_cli_subscribe(cli, MSG_LOCK);
            flora_cli_subscribe(cli, MSG_ALARM);
            flora_cli_subscribe(cli, MSG_BATTERY);
            flora_cli_subscribe(cli, MSG_VOICE_COMMING);
            flora_cli_subscribe(cli, MSG_VOICE_END);
            flora_cli_subscribe(cli, MSG_VOICE_SLEEP);
            flora_cli_subscribe(cli, MSG_SYS_STATUS);
            pm_wakelock_init();
            e_power_idle_id = eloop_timer_add(power_idle_timeout_func, NULL, IDLE_TIMEOUT_S*1000, 0);
            pm_idle_entry(IDLE_ENTER);
            break;
        }
    }
    //pm_service_loop_run();
    eloop_run();

    return NULL;
}

