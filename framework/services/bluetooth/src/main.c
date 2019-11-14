#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>

#include <json-c/json.h>
#include <common.h>

#define RESTART_BSA_SERVER "/etc/init.d/bsa restart"

static struct bluetooth_module_t *g_bt_mod = NULL;
static struct bluetooth_device_t *g_bt_dev = NULL;

int bt_log_level = LEVEL_OVER_LOGI;
struct bt_service_handle *g_bt_handle = NULL;

#if defined(BT_SERVICE_IPC_BY_ZEROMQ)
static pthread_t p_bt_zeromq;
extern void *bt_zeromq_handle(void *args);
#elif defined(BT_SERVICE_IPC_BY_FLORA)
static pthread_t p_bt_flora;
extern void *bt_flora_handle(void *args);
#endif

static void bt_handle_destory(struct bt_service_handle *handle);

static inline int bluetooth_device_open (const hw_module_t *module, struct bluetooth_device_t **device) {
    return module->methods->open (module, BLUETOOTH_HARDWARE_MODULE_ID, (struct hw_device_t **) device);
}

void discovery_auto_connect(void *caller) {
    struct rk_bluetooth *bt = caller;

    auto_connect_by_scan_results(bt);

}

static void discovery_callback(void *caller, const char *bt_name, BTAddr bt_addr, int is_completed, void *data) {

    a2dpsource_upload_scan_results(bt_name, bt_addr, is_completed, data);
    bt_upload_scan_results(bt_name, bt_addr, is_completed, data);
    if (is_completed) {
        discovery_auto_connect(caller);
        g_bt_handle->scanning = 0;
    }
}

static void bluetooth_device_destory(struct bt_service_handle *handle) {
    if (handle->bt) {
        g_bt_dev->destroy(handle->bt);
        handle->bt = NULL;
    }
}

void manage_listener(void *caller, BT_MGR_EVT event, void *data) {
    BT_MGT_MSG  *msg = data;

    switch (event) {
    case BT_EVENT_MGR_CONNECT:
        if (msg)
            BT_LOGI("BT_EVENT_MGR_CONNECT : enable= %d\n", msg->connect.enable);
        break;
    case BT_EVENT_MGR_DISCONNECT:
        if (msg)
            BT_LOGW("BT_EVENT_MGR_DISCONNECT : reason= %d\n", msg->disconnect.reason);

        //todo upload failed msg
        //bt_handle_destory(g_bt_handle);
        //sleep(2);
        // when bsa && bluetoothd closed, bluetooth_service restart
        exit(-2);
        break;
    case BT_EVENT_MGR_REMOVE_DEVICE:
        if (msg) {
            broadcast_remove_dev(msg->remove.address, msg->remove.status);
        }
        break;
    default:
        break;
    }
    return;
}

static int bluetooth_device_create(struct bt_service_handle *handle) {
    int ret = 0;
    struct rk_bluetooth *bt = NULL;
    int times = 0;

    if (handle->bt) {
        BT_LOGI("bt is not destory\n");
        return -1;
    }

    ret = g_bt_dev->creat(&bt);
    if (ret || !bt) {
        BT_LOGE("failed to creat bt\n");
        return -2;
    }
    handle->bt = bt;

    bt->set_discovery_listener(bt, discovery_callback, bt);
    bt->set_manage_listener(bt, manage_listener, bt);
    while (1) {
        if (times > 2) {
            system(RESTART_BSA_SERVER);//open failed, we try to restart bsa server
        }
        ret = bt_open(handle, NULL);
        if (ret) {
            BT_LOGW("open failed ret :: %d\n", ret);
            sleep(2);
            times++;
        } else {
            usleep(50 * 1000);
            bt_close(handle);
            return 0;
        }
    }

    return ret;
}

static int bluetooth_device_init(struct bt_service_handle *handle) {
    int ret;

    if (hw_get_module(BLUETOOTH_HARDWARE_MODULE_ID, (const struct hw_module_t **) &g_bt_mod) == 0) {
        //open bluetooth
        if (0 != bluetooth_device_open(&g_bt_mod->common, &g_bt_dev)) {
            BT_LOGE("failed to hw module: %s  device open\n", BLUETOOTH_HARDWARE_MODULE_ID);
            return -1;
        }
    } else {
        BT_LOGE("failed to fetch hw module: %s\n", BLUETOOTH_HARDWARE_MODULE_ID);
        return -1;
    }

    ret = bluetooth_device_create(handle);
    if (ret) {
        goto exit;
    }

    return 0;
exit:
    if (g_bt_dev) {
        bluetooth_device_destory(handle);
        g_bt_dev->common.close((struct hw_device_t *)g_bt_dev);
        g_bt_dev = NULL;
    }
    return ret;
}

int bt_handle_init(struct bt_service_handle *handle) {
    int ret = 0;
    struct bt_autoconnect_config *a2dpsink_config;
    struct bt_autoconnect_config *a2dpsource_config;

    a2dpsink_config = &(handle->bt_a2dpsink.config);

    strncpy(a2dpsink_config->autoconnect_filename, ROKIDOS_BT_A2DPSINK_AUTOCONNECT_FILE, strlen(ROKIDOS_BT_A2DPSINK_AUTOCONNECT_FILE));
    a2dpsink_config->autoconnect_num = ROKIDOS_BT_A2DPSINK_AUTOCONNECT_NUM;
    // init bluetooth config
    ret = bt_autoconnect_config_init(a2dpsink_config);
    if (ret < 0) {
        BT_LOGE("create bluetooth a2dpsink config %d\n", ret);
        return -1;
    }

    BT_LOGV("num :: %d\n", a2dpsink_config->autoconnect_num);
    BT_LOGV("linknum :: %d\n", a2dpsink_config->autoconnect_linknum);
    BT_LOGV("mode :: %d\n", a2dpsink_config->autoconnect_mode);

    a2dpsource_config = &(handle->bt_a2dpsource.config);

    a2dpsource_config->autoconnect_num = ROKIDOS_BT_A2DPSOURCE_AUTOCONNECT_NUM;

    strncpy(a2dpsource_config->autoconnect_filename, ROKIDOS_BT_A2DPSOURCE_AUTOCONNECT_FILE, strlen(ROKIDOS_BT_A2DPSOURCE_AUTOCONNECT_FILE));
    // init bluetooth config
    ret = bt_autoconnect_config_init(a2dpsource_config);
    if (ret < 0) {
        BT_LOGE("create bluetooth a2dpsource config %d\n", ret);
        return -1;
    }

    BT_LOGV("num :: %d\n", a2dpsource_config->autoconnect_num);
    BT_LOGV("linknum :: %d\n", a2dpsource_config->autoconnect_linknum);
    BT_LOGV("mode :: %d\n", a2dpsource_config->autoconnect_mode);

    g_bt_ble = &handle->bt_ble;
    g_bt_ble_client = &handle->bt_ble_client;
    g_bt_a2dpsink = &handle->bt_a2dpsink;
    g_bt_a2dpsource = &handle->bt_a2dpsource;
#if defined(BT_SERVICE_HAVE_HFP)
    g_bt_hfp = &handle->bt_hfp;
#endif
    g_bt_hh = &handle->bt_hh;
    eloop_init();

    ret = bt_a2dpsink_timer_init();
    if (ret < 0) {
        BT_LOGE("create bluetooth a2dpsink timer %d\n", ret);
        return -2;
    }

    ret = bt_a2dpsource_timer_init();
    if (ret < 0) {
        BT_LOGE("create bluetooth a2dpsource timer %d\n", ret);
        return -2;
    }

    return 0;
}

static void bt_handle_uninit(struct bt_service_handle *handle) {
    bt_a2dpsource_timer_uninit();
    bt_a2dpsink_timer_uninit();
    bt_autconnect_config_uninit(&handle->bt_a2dpsource.config);
    bt_autconnect_config_uninit(&handle->bt_a2dpsink.config);
}

static int bt_handle_create(struct bt_service_handle **handle) {

    if (*handle) {
        BT_LOGW("bt handle has init \n");
        return -1;
    }

    *handle = calloc(1, sizeof(struct bt_service_handle));
    if (*handle == NULL) {
        BT_LOGE("bt handle calloc error \n");
        return -2;
    }

    pthread_mutex_init(&((*handle)->bt_ble.state_mutex), NULL);
    pthread_mutex_init(&((*handle)->bt_a2dpsink.state_mutex), NULL);
    pthread_mutex_init(&((*handle)->bt_a2dpsource.state_mutex), NULL);
#if defined(BT_SERVICE_HAVE_HFP)
    pthread_mutex_init(&((*handle)->bt_hfp.state_mutex), NULL);
#endif
    pthread_mutex_init(&((*handle)->bt_ble_client.state_mutex), NULL);
    pthread_mutex_init(&((*handle)->bt_hh.state_mutex), NULL);

    return 0;
}


void bt_close_all(struct bt_service_handle *handle)
{
    bt_close_mask_profile(handle, ~0);
}

static void bt_handle_destory(struct bt_service_handle *handle) {
    if (handle) {
        bt_close_all(handle);
        eloop_stop();
        eloop_exit();
        bt_handle_uninit(handle);

        bluetooth_device_destory(handle);

        pthread_mutex_destroy(&handle->bt_ble.state_mutex);
        pthread_mutex_destroy(&handle->bt_a2dpsink.state_mutex);
        pthread_mutex_destroy(&handle->bt_a2dpsource.state_mutex);
#if defined(BT_SERVICE_HAVE_HFP)
        pthread_mutex_destroy(&handle->bt_hfp.state_mutex);
#endif
        pthread_mutex_destroy(&handle->bt_ble_client.state_mutex);
        pthread_mutex_destroy(&handle->bt_hh.state_mutex);
        free(handle);
        g_bt_handle = NULL;
    }
}

int handle_module_sleep(json_object *obj, struct bt_service_handle *handle) {
    char *command = NULL;
    json_object *pm_cmd = NULL;

    if (is_error(obj)) {
        return -1;
    }

    if (json_object_object_get_ex(obj, "sleep", &pm_cmd) == TRUE) {
        command = (char *)json_object_get_string(pm_cmd);
        BT_LOGI("pms :: sleep %s \n", command);

        if (strcmp(command, "ON") == 0) {
            bt_close_all(handle);
        }
    }
    return 0;
}

int handle_power_sleep(json_object *obj, struct bt_service_handle *handle) {
    char *command = NULL;
    json_object *pm_cmd = NULL;

    if (is_error(obj)) {
        return -1;
    }

    if (json_object_object_get_ex(obj, "sleep", &pm_cmd) == TRUE) {
        command = (char *)json_object_get_string(pm_cmd);
        BT_LOGI("pms :: sleep %s \n", command);

        if (strcmp(command, "OFF") == 0) {
            system(RESTART_BSA_SERVER);//for rokidme sleep out ,need restart bsa
        }
    }
    return 0;
}

int main(int argc, char *argv[argc]) {
    int ret = 0;

    ret = bt_handle_create(&g_bt_handle);
    if (ret < 0) {
        BT_LOGE("create bluetooth device error %d\n", ret);
        return -1;
    }

    ret = bluetooth_device_init(g_bt_handle);
    if (ret < 0) {
        BT_LOGE("create bluetooth device error %d\n", ret);
        return -2;
    }

    ret = bt_handle_init(g_bt_handle);
    if (ret < 0) {
        BT_LOGE("init bluetooth device error %d\n", ret);
        return -3;
    }


#if defined(BT_SERVICE_IPC_BY_ZEROMQ)
    ret = pthread_create(&p_bt_zeromq, NULL, bt_zeromq_handle, NULL);
    if (ret != 0) {
        BT_LOGE("can't create thread: %s\n", strerror(ret));
        return -4;
    }

    pthread_join(p_bt_zeromq, NULL);
#elif defined(BT_SERVICE_IPC_BY_FLORA)
    ret = pthread_create(&p_bt_flora, NULL, bt_flora_handle, NULL);
    if (ret != 0) {
        BT_LOGE("can't create thread: %s\n", strerror(ret));
        return -4;
    }

    pthread_join(p_bt_flora, NULL);
#endif
    bt_handle_destory(g_bt_handle);

    return 0;
}
