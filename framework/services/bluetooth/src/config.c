#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <json-c/json.h>

#include <common.h>
#include <config.h>

int bd_strtoba(uint8_t *addr, const char *address) {
    int i;
    int len = strlen(address);
    char *dest = (char *)addr;
    char *begin = (char *)address;
    char *end_ptr;

    if (!address || !addr || len != 17) {
        BT_LOGE("faile to addr:%s, len:%d\n", address, len);
        return -1;
    }
    for (i = 0; i < 6; i++) {
        dest[i] = (char)strtoul(begin, &end_ptr, 16);
        if (!end_ptr) break;
        if (*end_ptr == '\0') break;
        if (*end_ptr != ':') {
            BT_LOGE("faile to addr:%s, len:%d, end_ptr: %c, %s\n", address, len, *end_ptr, end_ptr);
            return -1;
        }
        begin = end_ptr +1;
        end_ptr = NULL;
    }
    if (i != 5) {
        BT_LOGE("faile to addr:%s, len:%d\n", address, len);
        return -1;
    }
    BT_LOGV("%02X:%02X:%02X:%02X:%02X:%02X\n",
           dest[0], dest[1], dest[2], dest[3], dest[4], dest[5]);
    return 0;
}

int bt_autoconnect_sync(struct bt_autoconnect_config *config) {
    int fd = 0;
    char *buf = NULL;
    char bt_addr[18] = {0};
    int i = 0;

    if (config == NULL) {
        BT_LOGE("config is null\n");
        return -1;
    }
    json_object *root = json_object_new_object();
    json_object *array = json_object_new_array();
    json_object *item[ROKIDOS_BT_AUTOCONNECT_DEVICES_MAX_NUM] = {0};

    json_object_object_add(root, "autoconnect_num", json_object_new_int(config->autoconnect_num));
    json_object_object_add(root, "autoconnect_mode", json_object_new_int(config->autoconnect_mode));

    for (i = 0; i < config->autoconnect_num; i++) {
        memset(bt_addr, 0, sizeof(bt_addr));

        item[i] = json_object_new_object();
        BT_LOGI("index : %d name :: %s\n", i, config->dev[i].name);
        BT_LOGI("addr :: %02x:%02x:%02x:%02x:%02x:%02x\n",
               config->dev[i].addr[0], config->dev[i].addr[1],
               config->dev[i].addr[2], config->dev[i].addr[3],
               config->dev[i].addr[4], config->dev[i].addr[5]);

        sprintf(bt_addr, "%02x:%02x:%02x:%02x:%02x:%02x",
                config->dev[i].addr[0], config->dev[i].addr[1],
                config->dev[i].addr[2], config->dev[i].addr[3],
                config->dev[i].addr[4], config->dev[i].addr[5]);


        json_object_object_add(item[i], "addr", json_object_new_string(bt_addr));
        json_object_object_add(item[i], "name", json_object_new_string(config->dev[i].name));

        json_object_array_add(array, item[i]);
    }

    json_object_object_add(root, "devices", array);

    unlink(config->autoconnect_filename);

    fd = open(config->autoconnect_filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        BT_LOGE("create %s error\n", config->autoconnect_filename);
        json_object_put(root);
        return -2;
    }

    buf = (char *)json_object_to_json_string(root);
    BT_LOGI("buf:: %s\n", buf);

    write(fd, buf, strlen(buf));

    fsync(fd);

    close(fd);

    json_object_put(root);

    return 0;
}

int bt_autoconnect_get_index(struct bt_autoconnect_config *config, struct bt_autoconnect_device *dev) {
    int i = 0;

    for (i = 0; i < config->autoconnect_num; i++) {
    #if 0
        BT_LOGV("config addr :: %02x:%02x:%02x:%02x:%02x:%02x\n",
               config->dev[i].addr[0], config->dev[i].addr[1],
               config->dev[i].addr[2], config->dev[i].addr[3],
               config->dev[i].addr[4], config->dev[i].addr[5]);

        BT_LOGV("addr :: %02x:%02x:%02x:%02x:%02x:%02x\n",
               dev->addr[0], dev->addr[1],
               dev->addr[2], dev->addr[3],
               dev->addr[4], dev->addr[5]);
    #endif
        if (memcmp(config->dev[i].addr, dev->addr, sizeof(config->dev[i].addr)) == 0) {
            return i;
        }
    }

    return -1;
}

int bt_autoconnect_update(struct bt_autoconnect_config *config, struct bt_autoconnect_device *dev) {
    int i = 0;
    int index = 0;

    index = bt_autoconnect_get_index(config, dev);
    if (index < 0) {
        if (config->autoconnect_num < 1) {
            return -1;
        }

        for (i = config->autoconnect_num - 1; i > 0; i--) {
            memcpy(&config->dev[i], &config->dev[i - 1], sizeof(struct bt_autoconnect_device));
        }

        memcpy(&config->dev[0], dev, sizeof(struct bt_autoconnect_device));

        if (config->autoconnect_linknum < config->autoconnect_num) {
            config->autoconnect_linknum++;
        }
    } else {
        for (i = index; i > 0; i--) {
            memcpy(&config->dev[i], &config->dev[i - 1], sizeof(struct bt_autoconnect_device));
        }

        memcpy(&config->dev[0], dev, sizeof(struct bt_autoconnect_device));
    }

    return 0;
}

int bt_autoconnect_remove(struct bt_autoconnect_config *config, struct bt_autoconnect_device *dev) {
    int i = 0;
    int index = 0;

    index = bt_autoconnect_get_index(config, dev);
    if (index >= 0) {
        for (i = index; i < config->autoconnect_num - 1; i++) {
            memcpy(&config->dev[i], &config->dev[i + 1], sizeof(struct bt_autoconnect_device));
        }
        memset(&config->dev[config->autoconnect_num - 1], 0x0, sizeof(struct bt_autoconnect_device));
        if (config->autoconnect_linknum > 0) {
            config->autoconnect_linknum--;
        }
    }

    return index >= 0 ? 0 : -1;
}

static int bt_config_newinit(struct bt_autoconnect_config *config) {
    config->autoconnect_mode = ROKIDOS_BT_AUTOCONNECT_MODE;
    // config->autoconnect_num = ROKIDOS_BT_AUTOCONNECT_NUM;
    config->autoconnect_linknum = 0;

    config->dev = calloc(config->autoconnect_num, sizeof(struct bt_autoconnect_device));
    if (config->dev == NULL) {
        BT_LOGE("malloc error\n");
        return -1;
    }

    return 0;
}

static int bt_config_parse(struct bt_autoconnect_config *config, json_object *root) {
    json_object *bt_autoconnect_mode = NULL;
    // json_object *bt_autoconnect_num = NULL;
    json_object *bt_autoconnect_devices = NULL;
    json_object *bt_dev = NULL;
    json_object *addr = NULL;
    json_object *name = NULL;
    int device_num = 0;
    int i = 0;
    char *bt_addr = NULL;
    char *bt_name = NULL;
    char zero_addr[6] = {0};

    if (json_object_object_get_ex(root, "autoconnect_mode", &bt_autoconnect_mode) == TRUE) {
        config->autoconnect_mode = ROKIDOS_BT_AUTOCONNECT_MODE;
    } else {
        config->autoconnect_mode = ROKIDOS_BT_AUTOCONNECT_MODE;
    }

    // if (json_object_object_get_ex(root, "autoconnect_num", &bt_autoconnect_num) == TRUE) {
    //     config->autoconnect_num = ROKIDOS_BT_AUTOCONNECT_NUM;
    // } else {
    //     config->autoconnect_num = ROKIDOS_BT_AUTOCONNECT_NUM;
    // }

    if (config->autoconnect_num >= ROKIDOS_BT_AUTOCONNECT_DEVICES_MAX_NUM) {
        config->autoconnect_num = ROKIDOS_BT_AUTOCONNECT_DEVICES_MAX_NUM;
    }

    config->dev = calloc(config->autoconnect_num, sizeof(struct bt_autoconnect_device));
    if (config->dev == NULL) {
        BT_LOGE("malloc error\n");
        return -1;
    }

    if (json_object_object_get_ex(root, "devices", &bt_autoconnect_devices) == TRUE) {
        device_num = json_object_array_length(bt_autoconnect_devices);
        BT_LOGI("device num :: %d\n", device_num);
        for (i = 0; i < device_num; i++) {
            bt_dev = json_object_array_get_idx(bt_autoconnect_devices, i);
            if (bt_dev != NULL) {
                BT_LOGI("index num :: %d\n", i);
                if (json_object_object_get_ex(bt_dev, "addr", &addr) == TRUE) {
                    bt_addr = (char *)json_object_get_string(addr);
                    BT_LOGI("addr %s\n", bt_addr);
                    if (bd_strtoba(config->dev[i].addr, bt_addr) != 0) {
                        BT_LOGW("error addr , init to zero");
                        memset(config->dev[i].addr, 0, sizeof(config->dev[i].addr));
                        continue;
                    }

                    if (memcmp(config->dev[i].addr, zero_addr, sizeof(zero_addr)) != 0) {
                        config->autoconnect_linknum++;
                    }

                } else {
                    BT_LOGW("no addr");
                    continue;
                }

                if (json_object_object_get_ex(bt_dev, "name", &name) == TRUE) {
                    bt_name = (char *)json_object_get_string(name);
                    strncpy(config->dev[i].name, bt_name, strlen(bt_name));
                } else {
                    BT_LOGW("no name");
                }
            }
        }
    }

    return 0;
}

int bt_autoconnect_config_init(struct bt_autoconnect_config *config) {
    int fd = 0;
    int ret = 0;
    char buf[1024] = {0};
    json_object *bt_config = NULL;

    fd = open(config->autoconnect_filename, O_RDONLY);
    if (fd < 0) {
        BT_LOGW("%s is not exist\n", config->autoconnect_filename);
        goto new_init;
    } else {
        ret = read(fd, buf, (sizeof(buf) - 1));
        close (fd);
        if (ret <= 0) {
            BT_LOGW("%s is not ok, newinit\n", config->autoconnect_filename);
            goto new_init;
        } else {
            BT_LOGI("%s\n", buf);
            bt_config = json_tokener_parse(buf);
            if (is_error(bt_config)) {
                BT_LOGW("%s is not right\n", config->autoconnect_filename);
                json_object_put(bt_config);
                goto new_init;
            } else {
                ret = bt_config_parse(config, bt_config);
            }
            json_object_put(bt_config);
        }
    }
    return ret;

new_init:
    return bt_config_newinit(config);
}


void bt_autconnect_config_uninit(struct bt_autoconnect_config *config) {
    if (config->dev) {
        free(config->dev);
        config->dev = NULL;
    }
}
