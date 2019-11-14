
#ifndef __BT_SERVICE_CONFIG_H_
#define __BT_SERVICE_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#define ROKIDOS_BT_AUTOCONNECT_BY_SCANRESULTS 0
#define ROKIDOS_BT_AUTOCONNECT_BY_HISTORY 1
#define ROKIDOS_BT_AUTOCONNECT_DEVICES_MAX_NUM 3


#ifndef ROKIDOS_BT_AUTOCONNECT_MODE
#define ROKIDOS_BT_AUTOCONNECT_MODE ROKIDOS_BT_AUTOCONNECT_BY_HISTORY
#endif

#ifndef ROKIDOS_BT_A2DPSINK_AUTOCONNECT_NUM
#define ROKIDOS_BT_A2DPSINK_AUTOCONNECT_NUM 2
#endif

#ifndef ROKIDOS_BT_A2DPSOURCE_AUTOCONNECT_NUM
#define ROKIDOS_BT_A2DPSOURCE_AUTOCONNECT_NUM 1
#endif

#define ROKIDOS_BT_A2DPSINK_AUTOCONNECT_FILE "/data/bluetooth/bt_a2dpsink.json"
#define ROKIDOS_BT_A2DPSOURCE_AUTOCONNECT_FILE "/data/bluetooth/bt_a2dpsource.json"

enum {
    AUTOCONNECT_BY_HISTORY_INIT = 99,
    AUTOCONNECT_BY_HISTORY_WORK,
    AUTOCONNECT_BY_HISTORY_OVER,
};

struct bt_autoconnect_device {
    char name[249];
    uint8_t addr[6];
};


struct bt_autoconnect_config {
    int autoconnect_index;//auto current conneting idx
    int autoconnect_mode;
    int autoconnect_num;
    int autoconnect_flag;
    int autoconnect_linknum;
    char autoconnect_filename[36];
    struct bt_autoconnect_device *dev;
};

int bd_strtoba(uint8_t *addr, const char *address);
int bt_autoconnect_sync(struct bt_autoconnect_config *config);
int bt_autoconnect_update(struct bt_autoconnect_config *config, struct bt_autoconnect_device *dev);
int bt_autoconnect_remove(struct bt_autoconnect_config *config, struct bt_autoconnect_device *dev);

int bt_autoconnect_config_init(struct bt_autoconnect_config *config);
void bt_autconnect_config_uninit(struct bt_autoconnect_config *config);
int bt_autoconnect_get_index(struct bt_autoconnect_config *config, struct bt_autoconnect_device *dev);
void bt_autoconnect_config_uninit(struct bt_autoconnect_config *config);

#ifdef __cplusplus
}
#endif
#endif
