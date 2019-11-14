#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include "ble_server.h"

#include <bsa_rokid/bsa_api.h>
#include <bsa_rokid/uipc.h>
#include "app_utils.h"
#include <bsa_rokid/btm_api.h>

#include "app_xml_utils.h"
#include "app_dm.h"
#include "ble.h"

#ifndef ROKIDOS_BOARDCONFIG_BLE_UUID_SERVICEID
#define ROKIDOS_BOARDCONFIG_BLE_UUID_SERVICEID 0xFFE1
#endif
#ifndef ROKIDOS_BOARDCONFIG_BLE_UUID_CHARACTER
#define ROKIDOS_BOARDCONFIG_BLE_UUID_CHARACTER 0x2A06
#endif

#define APP_BLE_MAIN_TEMP_UUID  0x1812

BleServer *_ble_server;

BleServer *ble_create(void *caller)
{
    BleServer *bles = calloc(1, sizeof(*bles));

    bles->caller = caller;
    _ble_server = bles;
    return bles;
}

int ble_destroy(BleServer *bles)
{
    if (bles) {
        //rokid_ble_disable(bles);
        free(bles);
    }
    _ble_server = NULL;
    return 0;
}

int app_ble_server_find_free_server(BleServer *bles)
{
    int index;

    for (index = 0; index < BSA_BLE_SERVER_MAX; index++)
    {
        if (!bles->ble_server[index].enabled)
        {
            return index;
        }
    }
    return -1;
}

UINT16 app_ble_get_server_uuid(BleServer *bles, int trans_id)
{
    int index = 0;
    int attr_num = 0;

    for (index = 0; index < BSA_BLE_SERVER_MAX; index++)
    {
        if (bles->ble_server[index].enabled)
        {
            BT_LOGI("%d:BLE Server server_if:%d\n", index,
                       bles->ble_server[index].server_if);
            for (attr_num = 0; attr_num < BSA_BLE_ATTRIBUTE_MAX ; attr_num++)
            {
                if (bles->ble_server[index].attr[attr_num].attr_UUID.uu.uuid16)
                {

                    if (bles->ble_server[index].attr[attr_num].attr_id == trans_id)
                    {
                        return bles->ble_server[index].attr[attr_num].attr_UUID.uu.uuid16;
                    }
                }
            }
        }
    }


    return -1;
}

int app_ble_get_server_transid(BleServer *bles, int uuid)
{
    int index = 0;
    int attr_num = 0;

    for (index = 0; index < BSA_BLE_SERVER_MAX; index++)
    {
        if (bles->ble_server[index].enabled)
        {
            BT_LOGI("%d:BLE Server server_if:%d\n", index,
                   bles->ble_server[index].server_if);
            for (attr_num = 0; attr_num < BSA_BLE_ATTRIBUTE_MAX ; attr_num++)
            {
                if (bles->ble_server[index].attr[attr_num].attr_UUID.uu.uuid16)
                {

                    if (bles->ble_server[index].attr[attr_num].attr_UUID.uu.uuid16 == uuid)
                    {
                        return bles->ble_server[index].attr[attr_num].attr_id;
                    }
                }
            }
        }
    }


    return -1;
}

void app_ble_server_display(BleServer *bles)
{
    int index, attr_num;

    BT_LOGI("*************** BLE SERVER LIST *****************\n");
    for (index = 0; index < BSA_BLE_SERVER_MAX; index++)
    {
        if (bles->ble_server[index].enabled)
        {
            BT_LOGI("%d:BLE Server server_if:%d\n", index,
                       bles->ble_server[index].server_if);
            for (attr_num = 0; attr_num < BSA_BLE_ATTRIBUTE_MAX ; attr_num++)
            {
                if (bles->ble_server[index].attr[attr_num].attr_UUID.uu.uuid16)
                {
                    if ((bles->ble_server[index].attr[attr_num].attr_type == BSA_GATTC_ATTR_TYPE_SRVC) ||
                       (bles->ble_server[index].attr[attr_num].attr_type == BSA_GATTC_ATTR_TYPE_INCL_SRVC))
                    {
                        BT_LOGI("\t attr_num:%d:uuid:0x%04x, is_pri:%d, service_id:%d attr_id:%d\n",
                            attr_num,
                            bles->ble_server[index].attr[attr_num].attr_UUID.uu.uuid16,
                            bles->ble_server[index].attr[attr_num].is_pri,
                            bles->ble_server[index].attr[attr_num].service_id,
                            bles->ble_server[index].attr[attr_num].attr_id);
                    }
                    else
                    {
                        BT_LOGI("\t\t attr_num:%d:uuid:0x%04x, is_pri:%d, service_id:%d attr_id:%d\n",
                            attr_num,
                            bles->ble_server[index].attr[attr_num].attr_UUID.uu.uuid16,
                            bles->ble_server[index].attr[attr_num].is_pri,
                            bles->ble_server[index].attr[attr_num].service_id,
                            bles->ble_server[index].attr[attr_num].attr_id);
                    }
                }
            }
        }
    }
    BT_LOGI("*************** BLE SERVER LIST END *************\n");
}

int app_ble_server_find_free_attr(BleServer *bles, UINT16 server)
{
    int index;

    for (index = 0; index < BSA_BLE_ATTRIBUTE_MAX; index++)
    {
        if (!bles->ble_server[server].attr[index].attr_UUID.uu.uuid16)
        {
            return index;
        }
    }
    return -1;
}

int app_ble_server_find_index_by_interface(BleServer *bles, tBSA_BLE_IF if_num)
{
    int index;

    for (index = 0; index < BSA_BLE_SERVER_MAX; index++)
    {
        if (bles->ble_server[index].server_if == if_num)
        {
            return index;
        }
    }
    return -1;
}

static void app_ble_server_profile_cback(tBSA_BLE_EVT event, tBSA_BLE_MSG *p_data);

int rokid_ble_enable(BleServer *bles)
{
    int ret;

    BT_CHECK_HANDLE(bles);
    if (bles->enabled) {
        BT_LOGW("ble already enabled!");
        return 0;
    }
    ret = ble_enable(BLE_SERVER_STATUS);
    if (ret) goto enable_err;

    ret = ble_server_register(bles, APP_BLE_MAIN_TEMP_UUID);
    if (ret) {
        goto register_err;
    }

    ret = ble_server_create_service(bles, 0);
    if (ret) {
        goto create_service_err;
    }
    ret = ble_server_add_char(bles, 0, 0, BLE_CHAR_ID1, 0);
    ret |= ble_server_add_char(bles, 0, 0, 0x2902, 1);
    ret |= ble_server_add_char(bles, 0, 0, BLE_CHAR_ID2, 0);
    ret |= ble_server_add_char(bles, 0, 0, 0x2902, 1);
    if (ret) goto create_service_err;

    ret = ble_server_start_service(bles, 0, 0);
    if (ret) {
        goto start_service_err;
    }

    ret = ble_server_start_adv(bles);
    if (ret) {
        goto start_adv_err;
    }

    app_dm_set_visibility(FALSE,FALSE);
    app_dm_set_ble_visibility(TRUE, TRUE);
    bles->enabled = TRUE;
    return ret;

start_adv_err:
    ble_server_stop_service(bles, 0, 0);
start_service_err:
create_service_err:
    ble_server_deregister(bles);
register_err:
    ble_disable(BLE_SERVER_STATUS);
enable_err:

    return ret;
}

int rokid_ble_disable(BleServer *bles)
{
    tBSA_STATUS status;

    BT_CHECK_HANDLE(bles);
    if(!bles->enabled) {
        BT_LOGW("ble not enabled!\n");
        return 0;
    }
    ble_server_stop_service(bles, 0, 0);
    ble_server_close(bles);
    ble_server_deregister(bles);
    status = ble_disable(BLE_SERVER_STATUS);
    memset(bles->ble_server, 0 , sizeof(bles->ble_server));
    bles->enabled = FALSE;
    return status;
}

int ble_get_enabled(BleServer *bles)
{
    return bles ? bles->enabled : 0;
}

int ble_server_set_menufacturer(BleServer *bles, char *name) {

    BT_CHECK_HANDLE(bles);
    if (!name) {
        return -1;
    }

    memset(bles->ble_menufacturer, 0, sizeof(bles->ble_menufacturer));

    if (strlen(name) > BSA_DM_BLE_AD_DATA_LEN - 1) {
        strncpy(bles->ble_menufacturer, name, BSA_DM_BLE_AD_DATA_LEN  - 1);
    } else {
        strncpy(bles->ble_menufacturer, name, strlen(name));
    }

    return 0;
}

int ble_server_register(BleServer *bles, UINT16 uuid)
{
    int server_num;
    tBSA_STATUS status;
    tBSA_BLE_SE_REGISTER ble_register_param;

    server_num = app_ble_server_find_free_server(bles);
    if (server_num < 0) {
        BT_LOGE("No more spaces\n");
        return -1;
    }

    status = BSA_BleSeAppRegisterInit(&ble_register_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("ERROR::BSA_BleSeAppRegisterInit failed status = %d\n", status);
        return -1;
    }

    ble_register_param.uuid.len = 2;
    ble_register_param.uuid.uu.uuid16 = uuid;
    ble_register_param.p_cback = app_ble_server_profile_cback;
    
    //BTM_BLE_SEC_NONE: No authentication and no encryption
    //BTM_BLE_SEC_ENCRYPT: encrypt the link with current key
    //BTM_BLE_SEC_ENCRYPT_NO_MITM: unauthenticated encryption
    //BTM_BLE_SEC_ENCRYPT_MITM: authenticated encryption
    //TODO
    //ble_register_param.sec_act = 0; //BTM_BLE_SEC_NONE;

    status = BSA_BleSeAppRegister(&ble_register_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("ERROR::BSA_BleSeAppRegister failed status = %d\n", status);
        return -1;
    }
    bles->ble_server[server_num].enabled = TRUE;
    bles->ble_server[server_num].server_if = ble_register_param.server_if;
    BT_LOGI("enabled:%d, server_if:%d", bles->ble_server[server_num].enabled, bles->ble_server[server_num].server_if);
    return 0;
}

int ble_server_deregister_signal(BleServer *bles, int num)
{
    tBSA_STATUS status;
    tBSA_BLE_SE_DEREGISTER ble_deregister_param;

    if (bles->ble_server[num].enabled != TRUE)
    {
        BT_LOGW("Server was not registered!\n");
        return -1;
    }

    status = BSA_BleSeAppDeregisterInit(&ble_deregister_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("ERROR::BSA_BleSeAppDeregisterInit failed status = %d\n", status);
        return -1;
    }

    ble_deregister_param.server_if = bles->ble_server[num].server_if;

    status = BSA_BleSeAppDeregister(&ble_deregister_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("ERROR::BSA_BleSeAppDeregister failed status = %d\n", status);
        return -1;
    }

    return 0;
}

int ble_server_deregister(BleServer *bles)
{
    int i = 0;

    for (i = 0; i < BSA_BLE_SERVER_MAX; i++) {
        ble_server_deregister_signal(bles, i);
    }

    return 0;
}

int ble_server_create_service(BleServer *bles, int server_num)
{
    tBSA_STATUS status;
    tBSA_BLE_SE_CREATE ble_create_param;
    UINT16 service;
    int attr_num;

    if ((server_num < 0) || (server_num >= BSA_BLE_SERVER_MAX))
    {
        BT_LOGE("Wrong server number! = %d", server_num);
        return -1;
    }

    if (bles->ble_server[server_num].enabled != TRUE)
    {
        BT_LOGE("ERROR::Server was not enabled!\n");
        return -1;
    }

    service = ROKIDOS_BOARDCONFIG_BLE_UUID_SERVICEID;//rokid define
    if (!service)
    {
        BT_LOGE("ERROR::wrong value = %d\n", service);
        return -1;
    }

    status = BSA_BleSeCreateServiceInit(&ble_create_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("ERROR::BSA_BleSeCreateServiceInit failed status = %d\n", status);
        return -1;
    }

    attr_num = app_ble_server_find_free_attr(bles, server_num);
    if (attr_num < 0) {
        BT_LOGE("Wrong attr number! = %d", attr_num);
        return -1;
    }

    ble_create_param.service_uuid.uu.uuid16 = service;
    ble_create_param.service_uuid.len = 2;
    ble_create_param.server_if = bles->ble_server[server_num].server_if;
    ble_create_param.num_handle = 12;;
    ble_create_param.is_primary = TRUE;

    bles->ble_server[server_num].attr[attr_num].wait_flag = TRUE;
    status = BSA_BleSeCreateService(&ble_create_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("ERROR::BSA_BleSeCreateService failed status = %d\n", status);
        bles->ble_server[server_num].attr[attr_num].wait_flag = FALSE;
        return -1;
    }

    while (bles->ble_server[server_num].attr[attr_num].wait_flag)
        usleep(10000);
    /* store information on control block */
    bles->ble_server[server_num].attr[attr_num].attr_UUID.len = 2;
    bles->ble_server[server_num].attr[attr_num].attr_UUID.uu.uuid16 = service;
    bles->ble_server[server_num].attr[attr_num].is_pri = ble_create_param.is_primary;
    bles->ble_server[server_num].attr[attr_num].attr_type = BSA_GATTC_ATTR_TYPE_SRVC;

    while (bles->ble_server[server_num].attr[attr_num].wait_flag == TRUE) {
        usleep(5000);
    }
    return 0;
}

int ble_server_add_char(BleServer *bles, int server_num, int attr_num, UINT16 uuid, int  is_descript)
{
    tBSA_STATUS status;
    tBSA_BLE_SE_ADDCHAR ble_addchar_param;
    int characteristic_property = 0;
    int char_attr_num = 0;

    if ((server_num < 0) || (server_num >= BSA_BLE_SERVER_MAX))
    {
        BT_LOGE("Wrong server number! = %d", server_num);
        return -1;
    }

    if (bles->ble_server[server_num].enabled != TRUE)
    {
        BT_LOGE("ERROR::Server was not enabled!\n");
        return -1;
    }

    // char_uuid = ROKIDOS_BOARDCONFIG_BLE_UUID_CHARACTER;//rokid define

    char_attr_num = app_ble_server_find_free_attr(bles, server_num);

    status = BSA_BleSeAddCharInit(&ble_addchar_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("ERROR::BSA_BleSeAddCharInit failed status = %d\n", status);
        return -1;
    }
    ble_addchar_param.service_id = bles->ble_server[server_num].attr[attr_num].service_id;
    ble_addchar_param.char_uuid.len = 2;
    ble_addchar_param.char_uuid.uu.uuid16 = uuid;

    if (is_descript) {
        ble_addchar_param.is_descr = TRUE;

        ble_addchar_param.perm = 0x11;
    } else {
        ble_addchar_param.is_descr = FALSE;
        ble_addchar_param.property = 0x3A;
        ble_addchar_param.perm = 0x11;

    }

    bles->ble_server[server_num].attr[char_attr_num].wait_flag = TRUE;
    
    status = BSA_BleSeAddChar(&ble_addchar_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("ERROR::BSA_BleSeAddChar failed status = %d\n", status);
        bles->ble_server[server_num].attr[char_attr_num].wait_flag = FALSE;
        return -1;
    }

    /* save all information */
    bles->ble_server[server_num].attr[char_attr_num].attr_UUID.len = 2;
    bles->ble_server[server_num].attr[char_attr_num].attr_UUID.uu.uuid16 = uuid;
    bles->ble_server[server_num].attr[char_attr_num].prop = characteristic_property;
    bles->ble_server[server_num].attr[char_attr_num].attr_type = BSA_GATTC_ATTR_TYPE_CHAR;
    // bles->ble_server[server_num].attr[char_attr_num].wait_flag = TRUE;
    while (bles->ble_server[server_num].attr[char_attr_num].wait_flag == TRUE){
        usleep(5000);
    }

    return 0;
}


int ble_server_start_service(BleServer *bles, int server_num, int attr_num)
{
    tBSA_STATUS status;
    tBSA_BLE_SE_START ble_start_param;

    if (bles->ble_server[server_num].enabled != TRUE)
    {
        BT_LOGE("### Server was not enabled!\n");
        return -1;
    }

    status = BSA_BleSeStartServiceInit(&ble_start_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("### BSA_BleSeStartServiceInit failed status = %d\n", status);
        return -1;
    }

    ble_start_param.service_id = bles->ble_server[server_num].attr[attr_num].service_id;
    ble_start_param.sup_transport = BSA_BLE_GATT_TRANSPORT_LE_BR_EDR;

    BT_LOGI("service_id:%d", ble_start_param.service_id);

    status = BSA_BleSeStartService(&ble_start_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("### BSA_BleSeStartService failed status = %d\n", status);
        return -1;
    }

    app_ble_server_display(bles);


    return 0;
}

int ble_server_start_adv(BleServer *bles)
{
    int i = 0;
    tBSA_DM_BLE_ADV_CONFIG adv_conf;
    UINT8 app_ble_adv_value[6] = {0x2b, 0x1a, 0xaa, 0xbb, 0xcc, 0xdd}; /*First 2 byte is Company Identifier Eg: 0x1a2b refers to Bluetooth ORG, followed by 4bytes of data*/

    /* This is just sample code to show how BLE Adv data can be sent from application */
    /*Adv.Data should be < 31bytes including Manufacturer data,Device Name, Appearance data, Services Info,etc.. */
    /* We are not receving all fields from user to reduce the complexity */
    memset(&adv_conf, 0, sizeof(tBSA_DM_BLE_ADV_CONFIG));
    /* start advertising */
    adv_conf.len = 6;
    adv_conf.flag = BSA_DM_BLE_ADV_FLAG_MASK;
    if (strlen(bles->ble_menufacturer) != 0) {
        memcpy(adv_conf.p_val, bles->ble_menufacturer, strlen(bles->ble_menufacturer));
        adv_conf.adv_data_mask = BSA_DM_BLE_AD_BIT_FLAGS|BSA_DM_BLE_AD_BIT_SERVICE|BSA_DM_BLE_AD_BIT_DEV_NAME|BSA_DM_BLE_AD_BIT_MANU;
    } else {
        memcpy(adv_conf.p_val, app_ble_adv_value, 6);
        adv_conf.adv_data_mask = BSA_DM_BLE_AD_BIT_FLAGS|BSA_DM_BLE_AD_BIT_SERVICE|BSA_DM_BLE_AD_BIT_DEV_NAME;
    }
    /* All the masks/fields that are set will be advertised*/
    //adv_conf.adv_data_mask = BSA_DM_BLE_AD_BIT_FLAGS|BSA_DM_BLE_AD_BIT_SERVICE|BSA_DM_BLE_AD_BIT_APPEARANCE|BSA_DM_BLE_AD_BIT_MANU;
    // adv_conf.adv_data_mask = BSA_DM_BLE_AD_BIT_DEV_NAME|BSA_DM_BLE_AD_BIT_FLAGS|BSA_DM_BLE_AD_BIT_SERVICE|BSA_DM_BLE_AD_BIT_APPEARANCE|BSA_DM_BLE_AD_BIT_MANU;
    // adv_conf.adv_data_mask = BSA_DM_BLE_AD_BIT_FLAGS|BSA_DM_BLE_AD_BIT_SERVICE|BSA_DM_BLE_AD_BIT_DEV_NAME|BSA_DM_BLE_AD_BIT_MANU;
    adv_conf.appearance_data = 65505;
    adv_conf.num_service = 1;
    for(i = 0; i < adv_conf.num_service; i++)
    {
        adv_conf.uuid_val[i] = 65505;
    }
    adv_conf.is_scan_rsp = 1;
    app_dm_set_ble_adv_data(&adv_conf);

    return 0;
}

int ble_server_stop_service(BleServer *bles, int num, int attr_num)
{
    tBSA_STATUS status;
    tBSA_BLE_SE_STOP ble_stop_param;

    if (bles->ble_server[num].enabled != TRUE)
    {
        BT_LOGE("### Server was not enabled!\n");
        return -1;
    }

    status = BSA_BleSeStopServiceInit(&ble_stop_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("### BSA_BleSeStopServiceInit failed status = %d\n", status);
        return -1;
    }

    ble_stop_param.service_id = bles->ble_server[num].attr[attr_num].service_id;

    BT_LOGI("stop service_id:%d", ble_stop_param.service_id);

    status = BSA_BleSeStopService(&ble_stop_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("### BSA_BleSeStopService failed status = %d", status);
        return -1;
    }
    return 0;
}


int rk_ble_send(BleServer *bles, UINT16 uuid, char *buf, int len)
{
    tBSA_STATUS status;
    tBSA_BLE_SE_SENDIND ble_sendind_param;
    int num = 0, length_of_data;

    BT_CHECK_HANDLE(bles);
    status = BSA_BleSeSendIndInit(&ble_sendind_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_BleSeSendIndInit failed status = %d\n", status);
        return -1;
    }



    ble_sendind_param.conn_id = bles->ble_server[num].conn_id;
    ble_sendind_param.attr_id = app_ble_get_server_transid(bles, uuid);


    if (ble_sendind_param.attr_id == -1)
    {
        BT_LOGE("uuid error\n");
        return -1;
    }

    length_of_data = len;
    ble_sendind_param.data_len = length_of_data;


    memcpy(ble_sendind_param.value, buf, len);

    ble_sendind_param.need_confirm = FALSE;

    status = BSA_BleSeSendInd(&ble_sendind_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_BleSeSendInd failed status = %d", status);
        return -1;
    }

    return 0;
}

// int rk_ble_send(BleServer *bles, char *buf, int len)
// {
//     tBSA_BLE_SE_SENDRSP send_server_resp;

//     BSA_BleSeSendRspInit(&send_server_resp);
//     send_server_resp.conn_id = bles->conn_id;
//     send_server_resp.trans_id = bles->trans_id;
//     send_server_resp.status = 0;

//     send_server_resp.len = len;
//     send_server_resp.auth_req = GATT_AUTH_REQ_NONE;
//     memcpy(send_server_resp.value, buf, len);

//     BSA_BleSeSendRsp(&send_server_resp);

//     return 0;
// }

static void app_ble_server_profile_cback(tBSA_BLE_EVT event, tBSA_BLE_MSG *p_data)
{
    int num, attr_index;
    //int status;
    tBSA_BLE_SE_SENDRSP send_server_resp;
    BT_BLE_MSG data = {0};

    BleServer *bles = _ble_server;

    BT_LOGD("\n event %d", event);
    switch (event)
    {
    case BSA_BLE_SE_DEREGISTER_EVT:
        BT_LOGI("BSA_BLE_SE_DEREGISTER_EVT server_if:%d status:%d",
            p_data->ser_deregister.server_if, p_data->ser_deregister.status);

        num = app_ble_server_find_index_by_interface(bles, p_data->ser_deregister.server_if);
        if (num < 0) {
            BT_LOGE("no deregister pending!!\n");
            break;
        }

        bles->ble_server[num].server_if = BSA_BLE_INVALID_IF;
        bles->ble_server[num].enabled = FALSE;

        for (attr_index = 0 ; attr_index < BSA_BLE_ATTRIBUTE_MAX ; attr_index++)
        {
            memset(&(bles->ble_server[num].attr[attr_index]), 0, sizeof(tAPP_BLE_ATTRIBUTE));
        }
        break;
    case BSA_BLE_SE_CREATE_EVT:
        BT_LOGI("BSA_BLE_SE_CREATE_EVT server_if:%d status:%d service_id:%d\n",
            p_data->ser_create.server_if, p_data->ser_create.status, p_data->ser_create.service_id);

        num = app_ble_server_find_index_by_interface(bles, p_data->ser_create.server_if);
        if(num < 0)
        {
            BT_LOGI("no interface!!\n");
            break;
        }

        /* search attribute number */
        for (attr_index = 0 ; attr_index < BSA_BLE_ATTRIBUTE_MAX ; attr_index++)
        {
            if (bles->ble_server[num].attr[attr_index].wait_flag == TRUE)
            {
                BT_LOGI("BSA_BLE_SE_CREATE_EVT if_num:%d, attr_num:%d\n", num, attr_index);
                if (p_data->ser_create.status == BSA_SUCCESS)
                {
                    bles->ble_server[num].attr[attr_index].service_id = p_data->ser_create.service_id;
                    bles->ble_server[num].attr[attr_index].wait_flag = FALSE;
                    break;
                }
                else  /* if CREATE fail */
                {
                    memset(&bles->ble_server[num].attr[attr_index], 0, sizeof(tAPP_BLE_ATTRIBUTE));
                    break;
                }
            }
        }
        if (attr_index >= BSA_BLE_ATTRIBUTE_MAX)
        {
            BT_LOGI("BSA_BLE_SE_CREATE_EVT no waiting!!\n");
            break;
        }
        break;
    case BSA_BLE_SE_ADDCHAR_EVT:
        BT_LOGI("BSA_BLE_SE_ADDCHAR_EVT status:%d\n", p_data->ser_addchar.status);
        BT_LOGI("attr_id:0x%x\n", p_data->ser_addchar.attr_id);
        num = app_ble_server_find_index_by_interface(bles, p_data->ser_addchar.server_if);
        if(num < 0)
        {
            BT_LOGI("no interface!!\n");
            break;
        }

        for (attr_index = 0 ; attr_index < BSA_BLE_ATTRIBUTE_MAX ; attr_index++)
        {
            if (bles->ble_server[num].attr[attr_index].wait_flag == TRUE)
            {
                BT_LOGI("if_num:%d, attr_num:%d\n", num, attr_index);
                if (p_data->ser_addchar.status == BSA_SUCCESS)
                {
                    bles->ble_server[num].attr[attr_index].service_id = p_data->ser_addchar.service_id;
                    bles->ble_server[num].attr[attr_index].attr_id = p_data->ser_addchar.attr_id;
                    bles->ble_server[num].attr[attr_index].wait_flag = FALSE;
                    break;
                }
                else  /* if CREATE fail */
                {
                    memset(&bles->ble_server[num].attr[attr_index], 0, sizeof(tAPP_BLE_ATTRIBUTE));
                    break;
                }
            }
        }
        if (attr_index >= BSA_BLE_ATTRIBUTE_MAX)
        {
            BT_LOGI("BSA_BLE_SE_ADDCHAR_EVT no waiting!!\n");
            break;
        }

        break;
    case BSA_BLE_SE_START_EVT:
        BT_LOGI("BSA_BLE_SE_START_EVT status:%d", p_data->ser_start.status);
        break;

    case BSA_BLE_SE_STOP_EVT:
        BT_LOGI("BSA_BLE_SE_STOP_EVT status:%d", p_data->ser_stop.status);
        break;
        // query form client
    case BSA_BLE_SE_READ_EVT:
        BT_LOGI("BSA_BLE_SE_READ_EVT status:%d", p_data->ser_read.status);
        BSA_BleSeSendRspInit(&send_server_resp);
        send_server_resp.conn_id = p_data->ser_read.conn_id;
        send_server_resp.trans_id = p_data->ser_read.trans_id;
        send_server_resp.status = p_data->ser_read.status;
        send_server_resp.handle = p_data->ser_read.handle;
        send_server_resp.offset = p_data->ser_read.offset;
        send_server_resp.len = 0;
        send_server_resp.auth_req = GATT_AUTH_REQ_NONE;
        // memcpy(send_server_resp.value, attribute_value, BSA_BLE_MAX_ATTR_LEN);
        BT_LOGI("BSA_BLE_SE_READ_EVT: send_server_resp.conn_id:%d, send_server_resp.trans_id:%d, %d", send_server_resp.conn_id, send_server_resp.trans_id, send_server_resp.status);
        BT_LOGI("BSA_BLE_SE_READ_EVT: send_server_resp.status:%d,send_server_resp.auth_req:%d", send_server_resp.status,send_server_resp.auth_req);
        BT_LOGI("BSA_BLE_SE_READ_EVT: send_server_resp.handle:%d, send_server_resp.offset:%d, send_server_resp.len:%d", send_server_resp.handle,send_server_resp.offset,send_server_resp.len );
        BSA_BleSeSendRsp(&send_server_resp);
        break;

    case BSA_BLE_SE_WRITE_EVT:
        BT_LOGI("BSA_BLE_SE_WRITE_EVT status:%d", p_data->ser_write.status);
        APP_DUMP("Write value", p_data->ser_write.value, p_data->ser_write.len);
        BT_LOGI("BSA_BLE_SE_WRITE_EVT trans_id:%d, conn_id:%d, handle:%d, is_prep:%d,offset:%d",
            p_data->ser_write.trans_id, p_data->ser_write.conn_id, p_data->ser_write.handle, 
            p_data->ser_write.is_prep, p_data->ser_write.offset);

        int write_uuid = app_ble_get_server_uuid(bles, p_data->ser_write.handle);
        BT_LOGI("trans id %04x\n", write_uuid);

        if (bles->listener) {
            data.ser_write.uuid = write_uuid;
            data.ser_write.len = p_data->ser_write.len;
            memcpy(data.ser_write.value, p_data->ser_write.value, BSA_BLE_MAX_ATTR_LEN);
            data.ser_write.offset = p_data->ser_write.offset;
            data.ser_write.status = p_data->ser_write.status;
            bles->listener(bles->listener_handle, BT_BLE_SE_WRITE_EVT, &data);
        }
        if (p_data->ser_write.need_rsp)
        {
            BSA_BleSeSendRspInit(&send_server_resp);
            send_server_resp.conn_id = p_data->ser_write.conn_id;
            send_server_resp.trans_id = p_data->ser_write.trans_id;
            send_server_resp.status = p_data->ser_write.status;
            send_server_resp.handle = p_data->ser_write.handle;
            if(p_data->ser_write.is_prep)
            {
                send_server_resp.offset = p_data->ser_write.offset;
                send_server_resp.len = p_data->ser_write.len;
                memcpy(send_server_resp.value, p_data->ser_write.value, send_server_resp.len);
            }
            else
                send_server_resp.len = 0;
            BT_LOGI("BSA_BLE_SE_WRITE_EVT: send_server_resp.conn_id:%d, send_server_resp.trans_id:%d, %d", send_server_resp.conn_id, send_server_resp.trans_id, send_server_resp.status);
            BT_LOGI("BSA_BLE_SE_WRITE_EVT: send_server_resp.status:%d,send_server_resp.auth_req:%d", send_server_resp.status,send_server_resp.auth_req);
            BT_LOGI("BSA_BLE_SE_WRITE_EVT: send_server_resp.handle:%d, send_server_resp.offset:%d, send_server_resp.len:%d", send_server_resp.handle,send_server_resp.offset,send_server_resp.len );
            BSA_BleSeSendRsp(&send_server_resp);
        }
        break;

    case BSA_BLE_SE_EXEC_WRITE_EVT:
        BT_LOGI("BSA_BLE_SE_EXEC_WRITE_EVT status:%d", p_data->ser_exec_write.status);
        BT_LOGI("BSA_BLE_SE_EXEC_WRITE_EVT trans_id:%d, conn_id:%d",
            p_data->ser_exec_write.trans_id, p_data->ser_exec_write.conn_id);

        BSA_BleSeSendRspInit(&send_server_resp);
        send_server_resp.conn_id = p_data->ser_exec_write.conn_id;
        send_server_resp.trans_id = p_data->ser_exec_write.trans_id;
        send_server_resp.status = p_data->ser_exec_write.status;
        send_server_resp.handle = 0;
        send_server_resp.len = 0;
        BT_LOGI("BSA_BLE_SE_WRITE_EVT: send_server_resp.conn_id:%d, send_server_resp.trans_id:%d, %d", send_server_resp.conn_id, send_server_resp.trans_id, send_server_resp.status);
        BT_LOGI("BSA_BLE_SE_WRITE_EVT: send_server_resp.status:%d", send_server_resp.status);
        BSA_BleSeSendRsp(&send_server_resp);

        break;

    case BSA_BLE_SE_OPEN_EVT:
        BT_LOGI("BSA_BLE_SE_OPEN_EVT status:%d", p_data->ser_open.reason);
        if (p_data->ser_open.reason == BSA_SUCCESS)
        {
            BT_LOGI("conn_id:0x%x", p_data->ser_open.conn_id);
            num = app_ble_server_find_index_by_interface(bles, p_data->ser_open.server_if);
            /* search interface number */
            if(num < 0)
            {
                BT_LOGE("no interface!!");
                break;
            }
            bles->ble_server[num].conn_id = p_data->ser_open.conn_id;


            //TODO
        #if 0
            /* XML Database update */
            app_read_xml_remote_devices();
            /* Add BLE service for this devices in XML database */
            app_xml_add_trusted_services_db(app_xml_remote_devices_db,
                    APP_NUM_ELEMENTS(app_xml_remote_devices_db), p_data->ser_open.remote_bda,
                    BSA_BLE_SERVICE_MASK);

            status = app_write_xml_remote_devices();
            if (status < 0)
            {
                BT_LOGI("### app_ble_write_remote_devices failed: %d", status);
            }
        #endif
        }
        if (bles->listener) {
            data.ser_open.reason = p_data->ser_open.reason;
            data.ser_open.conn_id = p_data->ser_open.conn_id;
            bles->listener(bles->listener_handle, BT_BLE_SE_OPEN_EVT, &data);
        }
        break;
    case BSA_BLE_SE_CONGEST_EVT:
        BT_LOGI("BSA_BLE_SE_CONGEST_EVT  :conn_id:0x%x, congested:%d",
            p_data->ser_congest.conn_id, p_data->ser_congest.congested);
        break;
    case BSA_BLE_SE_CLOSE_EVT:
        BT_LOGI("BSA_BLE_SE_CLOSE_EVT status:%d", p_data->ser_close.reason);
        BT_LOGI("conn_id:0x%x", p_data->ser_close.conn_id);
        num = app_ble_server_find_index_by_interface(bles, p_data->ser_close.server_if);
        if (bles->listener) {
            data.ser_close.reason = p_data->ser_close.reason;
            data.ser_close.conn_id = p_data->ser_close.conn_id;
            bles->listener(bles->listener_handle, BT_BLE_SE_CLOSE_EVT, &data);
        }
        bles->ble_server[num].conn_id = BSA_BLE_INVALID_CONN;
        break;
#if 0
    case BSA_BLE_SE_CONF_EVT:
        BT_LOGI("BSA_BLE_SE_CONF_EVT status:%d", p_data->ser_conf.status);
        BT_LOGI("conn_id:0x%x", p_data->ser_conf.conn_id);

        if (bles->listener)
        {
            data.ser_conf.status = p_data->ser_conf.status;
            data.ser_conf.conn_id = p_data->ser_conf.conn_id;
            bles->listener(bles->listener_handle, BT_BLE_SE_CONF_EVT, &data);
        }
#endif

        break;
    default:
        break;
    }
}

int ble_server_close(BleServer *bles)
{
    tBSA_STATUS status;
    tBSA_BLE_SE_CLOSE ble_close_param;
    int i = 0;
    int times = 3;

    for (i = 0; i < BSA_BLE_SERVER_MAX; i++) {
        if (bles->ble_server[i].enabled == FALSE) {
            BT_LOGI("no need close\n");
            continue;
        }

        status = BSA_BleSeCloseInit(&ble_close_param);
        if (status != BSA_SUCCESS) {
            BT_LOGE("BSA_BleSeCloseInit failed status = %d\n", status);
            return -1;
        }

        ble_close_param.conn_id = bles->ble_server[i].conn_id;
        status = BSA_BleSeClose(&ble_close_param);
        if (status != BSA_SUCCESS)
        {
            BT_LOGE("### BSA_BleSeClose failed status = %d\n", status);
            return -1;
        }
        while (bles->ble_server[i].conn_id != BSA_BLE_INVALID_CONN && times--)
            usleep(100 * 1000);
    }

    return 0;
}

int ble_server_set_listener(BleServer *bles, ble_callbacks_t listener, void *listener_handle)
{

    BT_CHECK_HANDLE(bles);
    bles->listener = listener;
    bles->listener_handle = listener_handle;
   return 0;
}
