/*****************************************************************************
 **
 **  Name:           app_hh.c
 **
 **  Description:    Bluetooth HID Host application
 **
 **  Copyright (c) 2009-2015, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/
/* for *printf */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include "hid_host.h"

#include "app_xml_utils.h"
#include "app_disc.h"
#include "app_utils.h"
#include "app_link.h"
#include "app_hh_db.h"

#if defined(APP_BTHID_INCLUDED) && (APP_BTHID_INCLUDED == TRUE)
#include "app_bthid.h"
#endif


#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
#include "app_ble_client_db.h"
#endif

#ifndef APP_HH_SEC_DEFAULT
#define APP_HH_SEC_DEFAULT BSA_SEC_NONE
#endif

/*
 * Globals
 */
static HidHost *HID_HOST;

/*
 * Local functions
 */
static tAPP_HH_DEVICE *app_hh_pdev_alloc(HidHost *hh, const BD_ADDR bda);
static tAPP_HH_DEVICE *app_hh_add_dev(HidHost *hh, tAPP_XML_REM_DEVICE *p_xml_dev);

/*******************************************************************************
 **
 ** Function         app_hh_pdev_find_by_handle
 **
 ** Description      Find HH known device by handle
 **
 ** Parameters       handle : HID handle to look for
 **
 ** Returns          Pointer to HH device structure (NULL if not found)
 **
 *******************************************************************************/
tAPP_HH_DEVICE *app_hh_pdev_find_by_handle(HidHost *hh, tBSA_HH_HANDLE handle)
{
    int cnt;

    for (cnt = 0; cnt < APP_NUM_ELEMENTS(hh->devices); cnt++)
    {
        if ((hh->devices[cnt].info_mask & APP_HH_DEV_USED) &&
            (hh->devices[cnt].handle == handle))
        {
            return &hh->devices[cnt];
        }
    }
    return NULL;
}

/*******************************************************************************
 **
 ** Function         app_hh_pdev_find_by_bdaddr
 **
 ** Description      Find HH known device by its BD address
 **
 ** Parameters       bda : BD address of the device to look for
 **
 ** Returns          Pointer to HH device structure (NULL if not found)
 **
 *******************************************************************************/
tAPP_HH_DEVICE *app_hh_pdev_find_by_bdaddr(HidHost *hh, const BD_ADDR bda)
{
    int cnt;

    for (cnt = 0 ; cnt < APP_NUM_ELEMENTS(hh->devices); cnt++)
    {
        if ((hh->devices[cnt].info_mask & APP_HH_DEV_USED) &&
            (bdcmp(bda, hh->devices[cnt].bd_addr) == 0))
        {
            return &hh->devices[cnt];
        }
    }
    return NULL;
}

/*******************************************************************************
 **
 ** Function         app_hh_pdev_alloc
 **
 ** Description      Allocate a new HH device
 **
 ** Parameters       bda : BD address of the new device to allocate
 **
 ** Returns          Pointer to HH device structure (NULL if not found)
 **
 *******************************************************************************/
static tAPP_HH_DEVICE *app_hh_pdev_alloc(HidHost *hh, const BD_ADDR bda)
{
    int cnt;
    tAPP_HH_DEVICE *p_hh_dev;

    for (cnt = 0 ; cnt < APP_NUM_ELEMENTS(hh->devices); cnt++)
    {
        if ((hh->devices[cnt].info_mask & APP_HH_DEV_USED) == 0)
        {
            p_hh_dev = &hh->devices[cnt];
            /* Mark device used */
            p_hh_dev->info_mask = APP_HH_DEV_USED;
            bdcpy(p_hh_dev->bd_addr, bda);
            p_hh_dev->handle = BSA_HH_INVALID_HANDLE;
            return p_hh_dev;
        }
    }
    BT_LOGE("Failed allocating new HH element");
    return NULL;
}

/*******************************************************************************
 **
 ** Function         app_hh_uipc_cback
 **
 ** Description      Process Report data
 **
 ** Parameters       p_msg : HID report
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_hh_uipc_cback(BT_HDR *p_msg)
{
    HidHost *hh = HID_HOST;
    tBSA_HH_UIPC_REPORT *p_uipc_report = (tBSA_HH_UIPC_REPORT *)p_msg;
#if ( defined (APP_BTHID_INCLUDED) && (APP_BTHID_INCLUDED == TRUE))
    tAPP_HH_DEVICE *p_hh_dev = app_hh_pdev_find_by_handle(hh, p_uipc_report->report.handle);
#endif


    BT_LOGI("app_hh_uipc_cback called Handle:%d BdAddr:%02X:%02X:%02X:%02X:%02X:%02X",
           p_uipc_report->report.handle,
           p_uipc_report->report.bd_addr[0], p_uipc_report->report.bd_addr[1],
           p_uipc_report->report.bd_addr[2], p_uipc_report->report.bd_addr[3],
           p_uipc_report->report.bd_addr[4], p_uipc_report->report.bd_addr[5]);
    BT_LOGI("\tMode:%d SubClass:0x%x CtryCode:%d len:%d event:%d",
            p_uipc_report->report.mode,
            p_uipc_report->report.sub_class,
            p_uipc_report->report.ctry_code,
            p_uipc_report->report.report_data.length,
            p_uipc_report->hdr.event);
#if 0
    if (hh->listener) {
        BT_HH_MSG msg;
        memcpy(msg.report.bd_addr, p_uipc_report->report.bd_addr, sizeof(msg.report.bd_addr));
        msg.report.report_data.data = p_uipc_report->report.report_data.data;
        msg.report.report_data.length = p_uipc_report->report.report_data.length;
        hh->listener(hh->listener_handle, BT_HH_REPORT_EVT, &msg);
    }
#endif
#if 0
    if (p_uipc_report->report.report_data.length > 32)
    {
        scru_dump_hex(p_uipc_report->report.report_data.data, "Report (truncated)",
                32, TRACE_LAYER_NONE, TRACE_TYPE_DEBUG);
    }
    else
    {
        scru_dump_hex(p_uipc_report->report.report_data.data, "Report",
                p_uipc_report->report.report_data.length, TRACE_LAYER_NONE, TRACE_TYPE_DEBUG);
    }
#endif

#if defined(APP_BTHID_INCLUDED) && (APP_BTHID_INCLUDED == TRUE)
    if ((p_hh_dev != NULL) && (p_hh_dev->bthid_fd != -1) )
    {
        app_bthid_report_write(p_hh_dev->bthid_fd,
            p_uipc_report->report.report_data.data,
            p_uipc_report->report.report_data.length);
    }
    else
    {
        BT_LOGE("bthid not opened for HH handle:%d", p_uipc_report->report.handle);
    }
#endif


    GKI_freebuf(p_msg);
}

/*******************************************************************************
 **
 ** Function         app_hh_cback
 **
 ** Description      Example of function to start HID Host application
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_hh_cback(tBSA_HH_EVT event, tBSA_HH_MSG *p_data)
{
    int index;
    tAPP_HH_DB_ELEMENT *p_hh_db_elmt;
    tAPP_HH_DEVICE *p_hh_dev;
    tAPP_XML_REM_DEVICE *p_xml_dev;
    tBSA_HH_CLOSE hh_close_param;
    HidHost *hh = HID_HOST;
    BT_HH_MSG msg;

    BT_LOGD("event(%d)", event);
    switch (event)
    {
    case BSA_HH_OPEN_EVT:
        BT_LOGI("BSA_HH_OPEN_EVT: status:%d, handle:%d mode:%s (%d)",
                p_data->open.status, p_data->open.handle,
                p_data->open.mode==BTA_HH_PROTO_RPT_MODE?"Report":"Boot",
                        p_data->open.mode);
        BT_LOGI("BdAddr :%02X-%02X-%02X-%02X-%02X-%02X",
                p_data->open.bd_addr[0], p_data->open.bd_addr[1], p_data->open.bd_addr[2],
                p_data->open.bd_addr[3], p_data->open.bd_addr[4], p_data->open.bd_addr[5]);

        BTDevice dev = {0};
        bdcpy(dev.bd_addr, p_data->open.bd_addr);
        if (rokidbt_find_scaned_device(hh->caller, &dev) == 1) {
            snprintf((char *)msg.open.name, sizeof(msg.open.name), "%s", dev.name);
        } else if (rokidbt_find_bonded_device(hh->caller, &dev) == 1) {
            snprintf((char *)msg.open.name, sizeof(msg.open.name), "%s", dev.name);
        }
        if (p_data->open.status == BSA_SUCCESS)
        {
            if (p_data->open.handle == hh->mip_handle)
            {
                hh->mip_device_nb++;
                BT_LOGI("%d 3DG MIP device connected", hh->mip_device_nb);
                break;
            }
            /* Check if the device is already in our internal database */
            p_hh_dev = app_hh_pdev_find_by_handle(hh, p_data->open.handle);
            if (p_hh_dev == NULL)
            {
                /* Add the device in the tables */
                p_hh_dev = app_hh_pdev_alloc(hh, p_data->open.bd_addr);
                if (p_hh_dev == NULL)
                {
                    BT_LOGE("Failed allocating HH element");
                    BSA_HhCloseInit(&hh_close_param);
                    hh_close_param.handle = p_data->open.handle;
                    BSA_HhClose(&hh_close_param);
                    break;
                }
                snprintf((char *)p_hh_dev->name, sizeof(p_hh_dev->name), "%s", dev.name);
                /* For safety, remove any previously existing HH db element */
                app_hh_db_remove_by_bda(p_data->open.bd_addr);

                p_hh_db_elmt = app_hh_db_alloc_element();
                if (p_hh_db_elmt != NULL)
                {
                    bdcpy(p_hh_db_elmt->bd_addr, p_data->open.bd_addr);
                    app_hh_db_add_element(p_hh_db_elmt);
                }
                else
                {
                    BT_LOGE("Failed allocating HH database element");
                    BSA_HhCloseInit(&hh_close_param);
                    hh_close_param.handle = p_data->open.handle;
                    BSA_HhClose(&hh_close_param);
                    break;
                }
                /* Copy the BRR info */
                p_hh_db_elmt->brr_size = sizeof(tBSA_HH_BRR_CFG);
                p_hh_db_elmt->brr = malloc(p_hh_db_elmt->brr_size);
                if (p_hh_db_elmt->brr != NULL)
                {
                    memcpy(p_hh_db_elmt->brr, &p_data->open.brr_cfg, p_hh_db_elmt->brr_size);
                }
                else
                {
                    p_hh_db_elmt->brr_size = 0;
                }

                p_hh_db_elmt->sub_class = p_data->open.sub_class;
                p_hh_db_elmt->attr_mask = p_data->open.attr_mask;

                /* Save the database */
                app_hh_db_save();

                /* Fetch the descriptor */
                app_hh_get_dscpinfo(p_data->open.handle);
            }

            /* Save the HH Handle */
            p_hh_dev->handle = p_data->open.handle;

#if (defined(BLE_INCLUDED_NOREADY) && BLE_INCLUDED_NOREADY == TRUE)
            p_hh_dev->le_hid = p_data->open.le_hid;
#endif
            /* Mark device opened */
            p_hh_dev->info_mask |= APP_HH_DEV_OPENED;
        }
        
        if (hh->listener) {
            msg.open.status = p_data->open.status;
            bdcpy(msg.open.bd_addr, p_data->open.bd_addr);
            msg.open.mode = p_data->open.mode;
            hh->listener(hh->listener_handle, BT_HH_OPEN_EVT, &msg);
        }
        break;

    /* coverity[MISSING BREAK] False-positive: Unplug event is handled as a close event */
    case BSA_HH_VC_UNPLUG_EVT:
        BT_LOGI("BSA_HH_VC_UNPLUG_EVT is handled like a close event, redirecting");

    case BSA_HH_CLOSE_EVT:
        BT_LOGI("BSA_HH_CLOSE_EVT: status:%d  handle:%d",
                p_data->close.status, p_data->close.handle);

        if (p_data->close.handle == hh->mip_handle)
        {
            if (hh->mip_device_nb > 0)
            {
                hh->mip_device_nb--;
            }
            BT_LOGI("%d 3DG MIP device connected", hh->mip_device_nb);
            break;
        }

        p_hh_dev = app_hh_pdev_find_by_handle(hh, p_data->close.handle);
        if (p_hh_dev != NULL)
        {
            p_hh_dev->info_mask &= ~APP_HH_DEV_OPENED;
            bdcpy(msg.close.bd_addr, p_hh_dev->bd_addr);
        }
        if (hh->listener) {
            msg.close.status = p_data->close.status;
            hh->listener(hh->listener_handle, BT_HH_CLOSE_EVT, &msg);
        }
        break;

    case BSA_HH_GET_DSCPINFO_EVT:
        BT_LOGI("BSA_HH_GET_DSCPINFO_EVT: status:%d handle:%d",
                p_data->dscpinfo.status, p_data->dscpinfo.handle);

        if (p_data->dscpinfo.status == BSA_SUCCESS)
        {
            BT_LOGI("DscpInfo: VID/PID (hex) = %04X/%04X",
                    p_data->dscpinfo.peerinfo.vendor_id,
                    p_data->dscpinfo.peerinfo.product_id);
            BT_LOGI("DscpInfo: version = %d", p_data->dscpinfo.peerinfo.version);
            BT_LOGI("DscpInfo: ssr_max_latency = %d ssr_min_tout = %d",
                    p_data->dscpinfo.peerinfo.ssr_max_latency,
                    p_data->dscpinfo.peerinfo.ssr_min_tout);
            BT_LOGI("DscpInfo: supervision_tout = %d", p_data->dscpinfo.peerinfo.supervision_tout);
            BT_LOGI("DscpInfo (descriptor len:%d):", p_data->dscpinfo.dscpdata.length);
            scru_dump_hex(p_data->dscpinfo.dscpdata.data, NULL,
                    p_data->dscpinfo.dscpdata.length, TRACE_LAYER_NONE, TRACE_TYPE_DEBUG);

            /* Check if the device is already in our internal database */
            p_hh_dev = app_hh_pdev_find_by_handle(hh, p_data->dscpinfo.handle);
            if (p_hh_dev)
            {
                BT_LOGI("Update Remote Device XML file");
                app_read_xml_remote_devices();
                app_xml_update_pidvid_db(app_xml_remote_devices_db,
                        APP_NUM_ELEMENTS(app_xml_remote_devices_db),
                        p_hh_dev->bd_addr,
                        p_data->dscpinfo.peerinfo.product_id,
                        p_data->dscpinfo.peerinfo.vendor_id);
                app_xml_update_version_db(app_xml_remote_devices_db,
                        APP_NUM_ELEMENTS(app_xml_remote_devices_db),
                        p_hh_dev->bd_addr,
                        p_data->dscpinfo.peerinfo.version);
                app_write_xml_remote_devices();

                /* Find the pointer to the DB element or allocate one */
                p_hh_db_elmt = app_hh_db_find_by_bda(p_hh_dev->bd_addr);
                if (p_hh_db_elmt != NULL)
                {
                    /* Free the previous descriptor if there is one */
                    if (p_hh_db_elmt->descriptor != NULL)
                    {
                        free(p_hh_db_elmt->descriptor);
                        p_hh_db_elmt->descriptor = NULL;
                    }
                    /* Save the HH descriptor information */
                    p_hh_db_elmt->ssr_max_latency = p_data->dscpinfo.peerinfo.ssr_max_latency;
                    p_hh_db_elmt->ssr_min_tout = p_data->dscpinfo.peerinfo.ssr_min_tout;
                    p_hh_db_elmt->supervision_tout = p_data->dscpinfo.peerinfo.supervision_tout;
                    p_hh_db_elmt->descriptor_size = p_data->dscpinfo.dscpdata.length;
                    if (p_hh_db_elmt->descriptor_size)
                    {
                        /* Copy the descriptor content */
                        p_hh_db_elmt->descriptor = malloc(p_hh_db_elmt->descriptor_size);
                        if (p_hh_db_elmt->descriptor  != NULL)
                        {
                            memcpy(p_hh_db_elmt->descriptor, p_data->dscpinfo.dscpdata.data, p_hh_db_elmt->descriptor_size);
                        }
                        else
                        {
                            p_hh_db_elmt->descriptor_size = 0;
                        }
                    }

                    /* Save the database */
                    app_hh_db_save();

                    for (index = 0; index < APP_NUM_ELEMENTS(app_xml_remote_devices_db); index++)
                    {
                        p_xml_dev = &app_xml_remote_devices_db[index];
                        if ((p_xml_dev->in_use != FALSE) &&
                            (bdcmp(p_xml_dev->bd_addr, p_hh_db_elmt->bd_addr) == 0))
                        {
                            /* Let's add this device to Server HH database to allow it to reconnect */
                            BT_LOGI("Adding HID Device:%s", p_xml_dev->name);
                            app_hh_add_dev(hh, p_xml_dev);
                            break;
                        }
                    }
                }
                else
                {
                    BT_LOGE("BSA_HH_GET_DSCPINFO_EVT: app_hh_db_find_by_bda failed");
                    BSA_HhCloseInit(&hh_close_param);
                    hh_close_param.handle = p_data->dscpinfo.handle;
                    BSA_HhClose(&hh_close_param);
                }
            }
        }
        break;

    case BSA_HH_GET_REPORT_EVT:
        BT_LOGI("BSA_HH_GET_REPORT_EVT: status:%d handle:%d len:%d",
                p_data->get_report.status, p_data->get_report.handle,
                p_data->get_report.report.length);

        if (p_data->get_report.report.length > 0)
        {
            BT_LOGI("Report ID: [%02x]",p_data->get_report.report.data[0]);

            scru_dump_hex(&p_data->get_report.report.data[1], "DATA",
                p_data->get_report.report.length-1, TRACE_LAYER_NONE, TRACE_TYPE_DEBUG);
        }
        break;

    case BSA_HH_SET_REPORT_EVT:
        BT_LOGI("BSA_HH_SET_REPORT_EVT: status:%d handle:%d",
                p_data->set_report.status, p_data->set_report.handle);
        break;

    case BSA_HH_GET_PROTO_EVT:
        BT_LOGI("BSA_HH_GET_PROTO_EVT: status:%d handle:%d mode:%d",
                p_data->get_proto.status, p_data->get_proto.handle,
                p_data->get_proto.mode);

        if (p_data->get_proto.status == BSA_SUCCESS)
        {
            /* Boot mode setting here is required to pass HH PTS test HOS_HID_BV_09_C*/
            app_hh_change_proto_mode(p_data->get_proto.handle,
                                     BSA_HH_PROTO_BOOT_MODE);
        }

        break;

    case BSA_HH_SET_PROTO_EVT:
        BT_LOGI("BSA_HH_SET_PROTO_EVT: status:%d, handle:%d",
                p_data->set_proto.status, p_data->set_proto.handle);
        break;

    case BSA_HH_GET_IDLE_EVT:
        BT_LOGI("BSA_HH_GET_IDLE_EVT: status:%d, handle:%d, idle:%d",
                p_data->get_idle.status, p_data->get_idle.handle, p_data->get_idle.idle);
        break;

    case BSA_HH_SET_IDLE_EVT:
        BT_LOGI("BSA_HH_SET_IDLE_EVT: status:%d, handle:%d",
                p_data->set_idle.status, p_data->set_idle.handle);
        break;

    case BSA_HH_MIP_START_EVT:
        BT_LOGI("BSA_HH_MIP_START_EVT");
        hh->mip_handle = p_data->mip_start.handle;
        break;

    case BSA_HH_MIP_STOP_EVT:
        BT_LOGI("BSA_HH_MIP_STOP_EVT");
        hh->mip_handle = p_data->mip_stop.handle;
        break;

    default:
        BT_LOGE("bad event:%d", event);
        break;
    }
}

/*******************************************************************************
 **
 ** Function         app_hh_add_dev
 **
 ** Description      Add an HID device in the internal tables of BSA
 **
 ** Parameters       index: index of the device in the remote entries XML table
 **
 ** Returns          Pointer to HH device structure (NULL if failed)
 **
 *******************************************************************************/
static tAPP_HH_DEVICE *app_hh_add_dev(HidHost *hh, tAPP_XML_REM_DEVICE *p_xml_dev)
{
    tAPP_HH_DEVICE *p_hh_dev;
    DEV_CLASS class_of_device;
    UINT8 *p_cod = &class_of_device[0];
    UINT8 major;
    UINT8 minor;
    UINT16 services;
    int status;
    tBSA_HH_ADD_DEV hh_add_dev_param;
    tAPP_HH_DB_ELEMENT *p_hh_db_elmt;
#if defined(APP_BTHID_INCLUDED) && (APP_BTHID_INCLUDED == TRUE)
    BOOLEAN desc_present = FALSE;
#endif

    /* Find the HH DB element */
    p_hh_db_elmt = app_hh_db_find_by_bda(p_xml_dev->bd_addr);
    if (p_hh_db_elmt == NULL)
    {
        BT_LOGE("Trying to add an HH device that is not in the HH database");
        return NULL;
    }

    /* Check if device is already in internal tables */
    p_hh_dev = app_hh_pdev_find_by_bdaddr(hh, p_xml_dev->bd_addr);
    if (p_hh_dev == NULL)
    {
        /* Add the device in the tables */
        p_hh_dev = app_hh_pdev_alloc(hh, p_xml_dev->bd_addr);
        if (p_hh_dev == NULL)
        {
            return NULL;
        }
    }

    /* Add device in HH tables */
    BSA_HhAddDevInit(&hh_add_dev_param);

    bdcpy(hh_add_dev_param.bd_addr, p_hh_db_elmt->bd_addr);

    BT_LOGI("Attribute mask = %x - sub_class = %x",p_hh_db_elmt->attr_mask,p_hh_db_elmt->sub_class);
    hh_add_dev_param.attr_mask = p_hh_db_elmt->attr_mask;
    hh_add_dev_param.sub_class = p_hh_db_elmt->sub_class;
    hh_add_dev_param.peerinfo.ssr_max_latency = p_hh_db_elmt->ssr_max_latency;
    hh_add_dev_param.peerinfo.ssr_min_tout = p_hh_db_elmt->ssr_min_tout;
    hh_add_dev_param.peerinfo.supervision_tout = p_hh_db_elmt->supervision_tout;
    hh_add_dev_param.attr_mask |= HID_SUP_TOUT_AVLBL;

    hh_add_dev_param.peerinfo.vendor_id = p_xml_dev->vid;
    hh_add_dev_param.peerinfo.product_id = p_xml_dev->pid;
    hh_add_dev_param.peerinfo.version = p_xml_dev->version;

    /* Copy the descriptor if available */
    if ((p_hh_db_elmt->descriptor_size != 0) &&
        (p_hh_db_elmt->descriptor_size <= sizeof(hh_add_dev_param.dscp_data.data)) &&
        (p_hh_db_elmt->descriptor != NULL))
    {
        hh_add_dev_param.dscp_data.length = p_hh_db_elmt->descriptor_size;
        memcpy(hh_add_dev_param.dscp_data.data, p_hh_db_elmt->descriptor,
                p_hh_db_elmt->descriptor_size);
#if defined(APP_BTHID_INCLUDED) && (APP_BTHID_INCLUDED == TRUE)
        desc_present = TRUE;
#endif
    }

    status = BSA_HhAddDev(&hh_add_dev_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("Unable to add HID device:%d", status);
        app_hh_db_remove_by_bda(p_xml_dev->bd_addr);
        p_hh_dev->info_mask = 0;
        p_xml_dev->trusted_services &= ~BSA_HID_SERVICE_MASK;
        app_write_xml_remote_devices();
        return NULL;
    }

    /* Save the HH handle in the internal table */
    p_hh_dev->handle = hh_add_dev_param.handle;

    /* Add HID to security trusted services */
    p_xml_dev->trusted_services |= BSA_HID_SERVICE_MASK;
    app_write_xml_remote_devices();
    BT_LOGI("Services = x%x", p_xml_dev->trusted_services);

    /* Extract Minor and Major numbers from COD */
    p_cod = p_xml_dev->class_of_device;
    BTM_COD_MINOR_CLASS(minor, p_cod);
    BTM_COD_MAJOR_CLASS(major, p_cod);
    BT_LOGI("COD:%02X-%02X-%02X => %s", p_cod[0], p_cod[1], p_cod[2], app_get_cod_string(p_cod));

    /* Check if the COD means RemoteControl */
    if ((major == BTM_COD_MAJOR_PERIPHERAL) &&
        ((minor & ~BTM_COD_MINOR_COMBO) == BTM_COD_MINOR_REMOTE_CONTROL))
    {
        BTM_COD_SERVICE_CLASS(services, p_cod);
        BT_LOGI("This device is a RemoteControl");
#if defined(APP_HH_AUDIO_INCLUDED) && (APP_HH_AUDIO_INCLUDED == TRUE)
         /* If Service audio or Capturing bits set, this is a Voice capable RC */
         if (services & (BTM_COD_SERVICE_AUDIO | BTM_COD_SERVICE_CAPTURING))
         {
             p_hh_dev->audio.is_audio_active = FALSE;
             p_hh_dev->audio.is_audio_device = TRUE;
         }
         else
         {
             p_hh_dev->audio.is_audio_device = FALSE;
         }
#endif
    }
#if defined(APP_HH_BLE_AUDIO_INCLUDED) && (APP_HH_BLE_AUDIO_INCLUDED == TRUE)
    if ((major == 0) && (minor == 0) && (p_xml_dev->device_type == BT_DEVICE_TYPE_BLE))
    {
        BT_LOGI("This device is a BLE RemoteControl");
        p_hh_dev->audio.is_audio_active = FALSE;
        p_hh_dev->audio.is_ble_audio_device = TRUE;
    }
#endif

#if defined(APP_BTHID_INCLUDED) && (APP_BTHID_INCLUDED == TRUE)
    if (p_hh_dev->bthid_fd != -1)
    {
        app_bthid_close(p_hh_dev->bthid_fd);
    }

    /* Open BTHID driver */
    p_hh_dev->bthid_fd = app_bthid_open();
    if (p_hh_dev->bthid_fd < 0)
    {
        BT_LOGE("BTHID open failed");
    }
    else
    {
        app_bthid_config(p_hh_dev->bthid_fd, "BSA sample apps HID device",
                p_xml_dev->vid, p_xml_dev->pid, p_xml_dev->version, 0);
        if (desc_present)
        {
            /* Send HID descriptor to the kernel (via bthid module) */
            app_bthid_desc_write(p_hh_dev->bthid_fd,
                                (unsigned char *)p_hh_db_elmt->descriptor,
                                p_hh_db_elmt->descriptor_size);
        }
    }
#endif

    return p_hh_dev;
}

/*******************************************************************************
 **
 ** Function         app_hh_get_dscpinfo
 **
 ** Description      Example of function to connect DHCP Info of an HID device
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_hh_get_dscpinfo(UINT8 handle)
{
    int status;
    tBSA_HH_GET get_dscpinfo;

    BT_LOGI("Get HID DSCP Info");
    BSA_HhGetInit(&get_dscpinfo);
    get_dscpinfo.request = BSA_HH_DSCP_REQ;
    get_dscpinfo.handle = handle;
    status = BSA_HhGet(&get_dscpinfo);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_HhGet: Unable to get DSCP Info status:%d", status);
        return -1;
    }

    return 0;
}

int app_hh_open(HidHost *hh, BD_ADDR bd_addr, tBSA_HH_PROTO_MODE mode)
{
    int status = -1;
    tBSA_HH_OPEN hh_open_param;

    BSA_HhOpenInit(&hh_open_param);
    bdcpy(hh_open_param.bd_addr, bd_addr);
    hh_open_param.mode = mode;
    hh_open_param.sec_mask = BSA_SEC_AUTHENTICATION | BSA_SEC_ENCRYPTION;

    status = BSA_HhOpen(&hh_open_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_HhOpen failed: %d", status);
    }

    return status;
}

/*******************************************************************************
 **
 ** Function         app_hh_disconnect
 **
 ** Description      Example of HH Close
 **
 ** Parameters
 **
 ** Returns
 **
 *******************************************************************************/
int app_hh_disconnect(HidHost *hh, BD_ADDR bd_addr)
{
    int status;
    tAPP_HH_DEVICE *dev;
    tBSA_HH_CLOSE hh_close_param;

    dev = app_hh_pdev_find_by_bdaddr(hh, bd_addr);
    if (!dev) {
        BT_LOGW("BdAddr :%02X:%02X:%02X:%02X:%02X:%02X not connect",
                bd_addr[0], bd_addr[1], bd_addr[2],
                bd_addr[3], bd_addr[4], bd_addr[5]);
        return 0;
    }

    BSA_HhCloseInit(&hh_close_param);
    hh_close_param.handle = dev->handle;
    status = BSA_HhClose(&hh_close_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("Unable to disconnect HID device status:%d", status);
        return -1;
    }
    else
    {
        BT_LOGI("Disconnecting %02X:%02X:%02X:%02X:%02X:%02X from HID devices",
                bd_addr[0], bd_addr[1], bd_addr[2],
                bd_addr[3], bd_addr[4], bd_addr[5]);
    }

    return 0;
}

int app_hh_disconnect_all(HidHost *hh)
{
    int index;

    for (index = 0; index < APP_NUM_ELEMENTS(hh->devices); index++)
    {
        if (hh->devices[index].info_mask & APP_HH_DEV_OPENED)
        {
            app_hh_disconnect(hh, hh->devices[index].bd_addr);
        }
    }

    return 0;
}


int app_hh_get_connected_devices(HidHost *hh, BTDevice *dev_array, int arr_len)
{
    int count = 0;
    int index;

    for (index = 0; index < APP_NUM_ELEMENTS(hh->devices); index++)
    {
        if ((hh->devices[index].info_mask & APP_HH_DEV_OPENED) && (arr_len > 0) && dev_array)
        {
            BTDevice *dev = dev_array + index;
            bdcpy(dev->bd_addr, hh->devices[index].bd_addr);
            snprintf(dev->name, sizeof(dev->name), "%s", hh->devices[index].name);
            count++;
        }
        if (count >= arr_len) break;
    }

    return count;
}

/*******************************************************************************
 **
 ** Function         app_hh_set_idle
 **
 ** Description      Example of HH Set Idle
 **
 ** Parameters
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_hh_set_idle(void)
{
    int status;
    tBSA_HH_HANDLE handle;
    tBSA_HH_SET hh_set_param;

    BT_LOGI("Bluetooth HID Set Idle");
    handle = app_get_choice("Enter HID Handle");

    BSA_HhSetInit(&hh_set_param);
    hh_set_param.request = BSA_HH_IDLE_REQ;
    hh_set_param.handle = handle;
    hh_set_param.param.idle = 0;

    status = BSA_HhSet(&hh_set_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_HhSet fail status:%d", status);
    }

    return status;
}

/*******************************************************************************
 **
 ** Function         app_hh_get_idle
 **
 ** Description      Example of HH get idle
 **
 ** Parameters
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_hh_get_idle(void)
{
    int status;
    tBSA_HH_HANDLE handle;
    tBSA_HH_GET hh_get_param;

    BT_LOGI("Bluetooth HID Get Idle");
    handle = app_get_choice("Enter HID Handle");

    BSA_HhGetInit(&hh_get_param);

    hh_get_param.request = BSA_HH_IDLE_REQ;
    hh_get_param.handle = handle;

    status = BSA_HhGet(&hh_get_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_HhGet fail status:%d", status);
    }

    return status;
}


/*******************************************************************************
 **
 ** Function         app_hh_change_proto_mode
 **
 ** Description      Example of HH change protocol mode
 **
 ** Parameters
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_hh_change_proto_mode(tBSA_HH_HANDLE handle, tBSA_HH_PROTO_MODE mode)
{
    int status;
    tBSA_HH_SET hh_set_param;

    BT_LOGI("Bluetooth HID Change Proto Mode");
    BSA_HhSetInit(&hh_set_param);
    hh_set_param.request = BSA_HH_PROTO_REQ;
    hh_set_param.handle = handle;
    hh_set_param.param.mode = mode;

    status = BSA_HhSet(&hh_set_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_HhSet fail status:%d", status);
    }

    return status;
}

/*******************************************************************************
 **
 ** Function         app_hh_set_proto_mode
 **
 ** Description      Example of HH set protocol mode
 **
 ** Parameters
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_hh_set_proto_mode(void)
{
    int status;
    tBSA_HH_HANDLE handle;
    tBSA_HH_SET hh_set_param;
    int choice;

    BT_LOGI("Bluetooth HID Set Protocol mode");
    handle = app_get_choice("Enter HID Handle");

    BSA_HhSetInit(&hh_set_param);
    hh_set_param.request = BSA_HH_PROTO_REQ;
    hh_set_param.handle = handle;
    BT_LOGI("    Select Protocol Mode:");
    BT_LOGI("        0 Report Mode");
    BT_LOGI("        1 Boot Mode");
    choice = app_get_choice("Protocol Mode");
    if(choice == 0)
        hh_set_param.param.mode = BSA_HH_PROTO_RPT_MODE;
    else if (choice == 1)
        hh_set_param.param.mode = BSA_HH_PROTO_BOOT_MODE;
    else{
        BT_LOGE("Invalid choice");
        return -1;
    }

    status = BSA_HhSet(&hh_set_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_HhSet fail status:%d", status);
    }

    return status;
}

/*******************************************************************************
 **
 ** Function         app_hh_get_proto_mode
 **
 ** Description      Example of HH get protocol mode
 **
 ** Parameters
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_hh_get_proto_mode(void)
{
    int status;
    tBSA_HH_HANDLE handle;
    tBSA_HH_GET hh_get_param;

    BT_LOGI("Bluetooth HID Get Proto Mode");
    handle = app_get_choice("Enter HID Handle");

    BSA_HhGetInit(&hh_get_param);

    hh_get_param.request = BSA_HH_PROTO_REQ;
    hh_get_param.handle = handle;

    status = BSA_HhGet(&hh_get_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_HhGet fail status:%d", status);
    }

    return status;
}

/*******************************************************************************
 **
 ** Function         app_hh_enable_mip
 **
 ** Description      Example of MIP feature enabling
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_hh_enable_mip(void)
{
    int status;
    tBSA_HH_ADD_DEV hh_add_dev_param;

    BT_LOGI("app_hh_enable_mip");

    BSA_HhAddDevInit(&hh_add_dev_param);

    hh_add_dev_param.enable_mip = TRUE;

    status = BSA_HhAddDev(&hh_add_dev_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_HhAddDev: Unable to enable MIP feature status:%d", status);
        return -1;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_hh_disable_mip
 **
 ** Description      Example of MIP feature enabling
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_hh_disable_mip(void)
{
    int status;
    tBSA_HH_REMOVE_DEV hh_remove_dev_param;

    BT_LOGI("app_hh_enable_mip");

    BSA_HhRemoveDevInit(&hh_remove_dev_param);

    hh_remove_dev_param.disable_mip = TRUE;

    status = BSA_HhRemoveDev(&hh_remove_dev_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_HhRemoveDev: Unable to disable MIP feature:%d", status);
        return -1;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_hh_set_report
 **
 ** Description      Example of HH Set Report
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_hh_set_report(void)
{
    int status;
    tBSA_HH_HANDLE handle;
    tBSA_HH_SET hh_set_param;
    UINT16 report_type, report_id;


    BT_LOGI("Bluetooth HID Set Report");
    handle = app_get_choice("Enter HID Handle");
    report_type = 3;//app_get_choice("Report Type(0=RESRV,1=INPUT,2=OUTPUT,3=FEATURE)");
    report_id = 1;//app_get_choice("Enter report id");

    BSA_HhSetInit(&hh_set_param);
    hh_set_param.request = BSA_HH_REPORT_REQ;
    hh_set_param.handle = handle;
    hh_set_param.param.set_report.report_type = report_type;

    status = app_get_hex_data("Report data (e.g. 01 AB 54 ..)",
            &hh_set_param.param.set_report.report.data[1],  BSA_HH_REPORT_SIZE_MAX - 1);
    if (status < 0)
    {
        BT_LOGE("incorrect Data");
        return -1;
    }
    hh_set_param.param.set_report.report.length = (UINT16)status + 1;
    hh_set_param.param.set_report.report.data[0] = report_id; /* Report Id = 0x01 */
    status = BSA_HhSet(&hh_set_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_HhSet fail status:%d", status);
        return -1;
    }

    return 0;
}



/*******************************************************************************
 **
 ** Function         app_hh_send_vc_unplug
 **
 ** Description      Example of HH Send Control type
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_hh_send_vc_unplug(void)
{
    int status;
    tBSA_HH_HANDLE handle;
    tBSA_HH_SEND_CTRL param;


    BT_LOGI("Bluetooth HID Send Virtual Cable Unplug");
    handle = app_get_choice("Select HID Handle");

    BSA_HhSendCtrlInit(&param);
    param.ctrl_type = BSA_HH_CTRL_VIRTUAL_CABLE_UNPLUG;
    param.handle = handle;
    status = BSA_HhSendCtrl(&param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_HhSendCtrl fail status:%d", status);
        return -1;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_hh_send_hid_ctrl
 **
 ** Description      Example of HH Send HID Control
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_hh_send_hid_ctrl(void)
{
    int status;
    tBSA_HH_HANDLE handle;
    tBSA_HH_SEND_CTRL param;
    UINT8 ctrl_type;

    BT_LOGI("Bluetooth send HID Control");
    handle = app_get_choice("Select HID Handle");

    BSA_HhSendCtrlInit(&param);
    ctrl_type = app_get_choice("Message type(3:suspend, 4:exit_suspend, 5:virtual cable unplug");
    param.ctrl_type = ctrl_type;
    param.handle = handle;
    status = BSA_HhSendCtrl(&param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_HhSendCtrl fail status:%d", status);
        return -1;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_hh_send_ctrl_data
 **
 ** Description     Example of HH Send Report on control Channel (any size/id)
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hh_send_ctrl_data(void)
{
    int status;
    tBSA_HH_HANDLE handle;
    tBSA_HH_SET hh_set_param;
    int report_id;
    int report_type;


    BT_LOGI("Bluetooth HID Send Control Data");
    handle = app_get_choice("Enter HID Handle");

    report_type = app_get_choice("Report Type(0=RESRV,1=INPUT,2=OUTPUT,3=FEATURE)");
    if ((report_type < 0) || (report_type > BSA_HH_RPTT_FEATURE))
    {
        BT_LOGE("Bad Report Type:0x%x", report_type);
        return -1;
    }

    report_id = app_get_choice("ReportId");
    if ((report_id < 0) || (report_id > 0xFF))
    {
        BT_LOGE("Bad report Id:0x%x", report_id);
        return -1;
    }

    BSA_HhSetInit(&hh_set_param);
    hh_set_param.request = BSA_HH_REPORT_REQ;
    hh_set_param.handle = handle;
    hh_set_param.param.set_report.report_type = (UINT8)report_type;

    status = app_get_hex_data("Report data (e.g. 01 AB 54 ..)",
            &hh_set_param.param.set_report.report.data[1],  BSA_HH_REPORT_SIZE_MAX - 1);
    if (status < 0)
    {
        BT_LOGE("incorrect Data");
        return -1;
    }
    hh_set_param.param.set_report.report.length = (UINT16)status + 1;
    hh_set_param.param.set_report.report.data[0] = report_id; /* Report Id = 0x01 */

    status = BSA_HhSet(&hh_set_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_HhSet fail status:%d", status);
        return -1;
    }

    return status;
}

/*******************************************************************************
 **
 ** Function        app_hh_send_int_data
 **
 ** Description     Example of HH Send Report on Interrupt Channel (any size/id)
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hh_send_int_data(void)
{
    int status;
    tBSA_HH_HANDLE handle;
    int report_id;
    int length;
    int index;
    tBSA_HH_SEND_DATA hh_send_data_param;


    BT_LOGI("Bluetooth HID Send Interrupt Data");
    handle = app_get_choice("Enter HID Handle");
    report_id = app_get_choice("ReportId");

    if ((report_id < 0) || (report_id > 0xFF))
    {
        BT_LOGE("Bad report Id:0x%x", report_id);
        return -1;
    }

    length = app_get_choice("Data Length (including report ID)");
    if ((length < 1) || (length >= BSA_HH_DATA_SIZE_MAX))
    {
        BT_LOGE("Bad length Id:0x%x", length);
        return -1;
    }

    BSA_HhSendDataInit(&hh_send_data_param);
    hh_send_data_param.handle = handle;

    hh_send_data_param.data.length = (UINT16)length;
    hh_send_data_param.data.data[0] = (UINT8)report_id;

    for (index = 1 ; index < length ; index++)
    {
        hh_send_data_param.data.data[index] = (UINT8)index-1;
    }
    status = BSA_HhSendData(&hh_send_data_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_HhSendData fail status:%d", status);
        return -1;
    }

    return status;
}


/*******************************************************************************
 **
 ** Function        app_hh_get_report
 **
 ** Description     Example of HH Get Report
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hh_get_report(int handle)
{
    int status;
    //tBSA_HH_HANDLE handle;
    tBSA_HH_GET hh_get_param;
    UINT8 report_type;


    BT_LOGI("Bluetooth HID Get Report");
    //handle = app_get_choice("Enter HID Handle");
    report_type = 3;//app_get_choice("Report Type(0=RESRV,1=INPUT,2=OUTPUT,3=FEATURE)");
    BSA_HhGetInit(&hh_get_param);

    hh_get_param.request = BSA_HH_REPORT_REQ;
    hh_get_param.handle = handle;
    hh_get_param.param.get_report.report_type = report_type;

    hh_get_param.param.get_report.report_id = 1;//app_get_choice("Enter Report ID");

    status = BSA_HhGet(&hh_get_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_HhGet fail status:%d", status);
        return -1;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_hh_get_report_no_size
 **
 ** Description     Example of HH Get Report
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hh_get_report_no_size(void)
{
    int status;
    tBSA_HH_HANDLE handle;
    tBSA_HH_GET hh_get_param;


    BT_LOGI("Bluetooth HID Get Report w/o size info");
    handle = app_get_choice("Enter HID Handle");

    BSA_HhGetInit(&hh_get_param);

    hh_get_param.request = BSA_HH_REPORT_NO_SIZE_REQ;
    hh_get_param.handle = handle;
    hh_get_param.param.get_report.report_type = BSA_HH_RPTT_INPUT;

    hh_get_param.param.get_report.report_id = app_get_choice("Enter Report ID");

    status = BSA_HhGet(&hh_get_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_HhGet fail status:%d", status);
        return -1;
    }

    return 0;
}


/*******************************************************************************
 **
 ** Function         app_hh_set_ucd_brr_mode
 **
 ** Description      Example to set UCD/BRR mode (send data)
 **
 ** Parameters
 **
 ** Returns          status (0 if successful, error code otherwise)
 **
 *******************************************************************************/
int app_hh_set_ucd_brr_mode(void)
{
    tBSA_STATUS status;
    tBSA_HH_HANDLE handle;
    tBSA_HH_SEND_DATA hh_send_data_param;


    BT_LOGI("Bluetooth HID Send data (UCD mode)");
    handle = app_get_choice("Enter HID Handle");

    BSA_HhSendDataInit(&hh_send_data_param);
    hh_send_data_param.handle = handle;
    hh_send_data_param.data.data[0] = 0xFF;  /* Report Id = 0xFF */
    hh_send_data_param.data.data[1] = 0x00;  /* Report data */
    hh_send_data_param.data.length = 2;      /* Report Length */

    status = BSA_HhSendData(&hh_send_data_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_HhSendData fail status:%d", status);
        return -1;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_hh_cfg_hid_ucd
 **
 ** Description      Configure the UCD flags over HID
 **
 ** Parameters       handle: handle to send to (if BSA_HH_INVALID_HANDLE, ask)
 **                  command: HID UCD command (if 0, ask)
 **                  flags: HID UCD flags (if 0, ask)
 **
 ** Returns          status (0 if successful, error code otherwise)
 **
 *******************************************************************************/
int app_hh_cfg_hid_ucd(UINT8 handle, UINT8 command, UINT8 flags)
{
    tBSA_STATUS status;
    tBSA_HH_SET hh_set_param;
    BT_LOGI("Set HID UCD mode");

    if (handle == BSA_HH_INVALID_HANDLE)
    {
        handle = app_get_choice("Enter HID handle");
    }

    if (!command)
    {
        command = app_get_choice("Enter HID UCD command (1=Set, 2=Clear, 3=Exec)");
    }

    if (!flags)
    {
        flags = app_get_choice("Enter flags to set (1=BRR, 2=UCD, 4=Multicast)");
    }


    BSA_HhSetInit(&hh_set_param);
    hh_set_param.request = BSA_HH_REPORT_REQ;
    hh_set_param.handle = handle;
    hh_set_param.param.set_report.report_type = BTA_HH_RPTT_FEATURE;
    hh_set_param.param.set_report.report.length = 3;
    hh_set_param.param.set_report.report.data[0] = 0xCC; /* Report Id = 0xCC */
    hh_set_param.param.set_report.report.data[1] = command; /* Set */
    hh_set_param.param.set_report.report.data[2] = flags; /* UCD */
    status = BSA_HhSet(&hh_set_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_HhSet fail status:%d", status);
        return -1;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_hh_init
 **
 ** Description      Initialize HH
 **
 ** Parameters       boot: indicate if the management path must be created
 **
 ** Returns          void
 **
 *******************************************************************************/
static int app_hh_init(HidHost *hh)
{
    memset(&hh->devices, 0, sizeof(hh->devices));
#if defined(APP_BTHID_INCLUDED) && (APP_BTHID_INCLUDED == TRUE)
    int index;
    for (index = 0 ; index < APP_HH_DEVICE_MAX; index++)
    {
        hh->devices[index].bthid_fd = -1;
    }
#endif
    hh->mip_device_nb = 0;
    hh->mip_handle = BSA_HH_MIP_INVALID_HANDLE;

    app_hh_db_init();

    return 0;
}


/*******************************************************************************
 **
 ** Function         app_hh_start
 **
 ** Description      Start HID Host
 **
 ** Parameters
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int hh_enable(HidHost *hh)
{
    int status;
    int index;
    tBSA_HH_ENABLE enable_param;
    tAPP_XML_REM_DEVICE *p_xml_dev;

    if (hh->enable) return 0;
    app_hh_init(hh);
    BSA_HhEnableInit(&enable_param);
    enable_param.sec_mask = APP_HH_SEC_DEFAULT;
    enable_param.p_cback = app_hh_cback;

    status = BSA_HhEnable(&enable_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("Unable to enable HID Host status:%d", status);
        return -1;
    }

    hh->uipc_channel = enable_param.uipc_channel; /* Save UIPC channel */
    UIPC_Open(hh->uipc_channel, app_hh_uipc_cback);  /* Open UIPC channel */

    /* Add every HID devices found in remote device database */
    /* They will be able to connect to our device */
    app_read_xml_remote_devices();
    for (index = 0; index < APP_NUM_ELEMENTS(app_xml_remote_devices_db); index++)
    {
        p_xml_dev = &app_xml_remote_devices_db[index];
        if ((p_xml_dev->in_use != FALSE) &&
            ((p_xml_dev->trusted_services & BSA_HID_SERVICE_MASK) != 0))
        {
            BT_LOGI("Adding HID Device:%s BdAddr:%02X:%02X:%02X:%02X:%02X:%02X",
                    p_xml_dev->name,
                    p_xml_dev->bd_addr[0], p_xml_dev->bd_addr[1], p_xml_dev->bd_addr[2],
                    p_xml_dev->bd_addr[3], p_xml_dev->bd_addr[4], p_xml_dev->bd_addr[5]);
            app_hh_add_dev(hh, p_xml_dev);
        }
    }
    hh->enable = true;
    return 0;
}

int hh_disable(HidHost *hh)
{
    tBSA_HH_DISABLE dis_param;

    BT_CHECK_HANDLE(hh);
    if (hh->enable) {
        app_hh_disconnect_all(hh);
        UIPC_Close(hh->uipc_channel);  /* Close UIPC channel */
        hh->uipc_channel = UIPC_CH_ID_BAD;
        hh->mip_device_nb = 0;
        hh->mip_handle = BSA_HH_MIP_INVALID_HANDLE;

        BSA_HhDisableInit(&dis_param);
        BSA_HhDisable(&dis_param);
        hh->enable = false;
    }
    app_hh_db_exit();

    return 0;
}

HidHost *hh_create(void *caller)
{
    HidHost *hh = calloc(1, sizeof(*hh));

    hh->caller = caller;
    //pthread_mutex_init(&hh->mutex, NULL);

    HID_HOST = hh;
    return hh;
}


int hh_destroy(HidHost *hh)
{
    if (hh) {
        //hh_disable(hh);
        //pthread_mutex_destroy(&hh->mutex);
        free(hh);
    }
    HID_HOST = NULL;
    return 0;
}

int hh_set_listener(HidHost *hh, hh_callbacks_t listener, void *listener_handle)
{
    BT_CHECK_HANDLE(hh);
    hh->listener = listener;
    hh->listener_handle = listener_handle;
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_hh_set_3dsg_mode
 **
 ** Description      Set 3DSG mode for one device
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_hh_set_3dsg_mode(BOOLEAN enable, int req_handle, int vsync_period)
{
    tBSA_HH_SET app_hh_set;
    tBSA_HH_HANDLE handle;
    tBSA_STATUS status;

    if (req_handle  == -1)
    {
        /* Show connected HID devices */
        BT_LOGI("Bluetooth HID Set 3DSG mode enable:%d", enable);
        handle = app_get_choice("Enter HID Handle");
    }
    else
    {
        handle = (tBSA_HH_HANDLE)req_handle;
    }

    BSA_HhSetInit(&app_hh_set);
    app_hh_set.handle = handle;
    if (enable)
    {
        app_hh_set.request = BSA_HH_3DSG_ON_REQ;
        BT_LOGI("These parameters are for %d ms VSync Period", vsync_period);
        app_hh_set.param.offset_3dsg.left_open = 0;
        app_hh_set.param.offset_3dsg.left_close = vsync_period / 2 - 1;
        app_hh_set.param.offset_3dsg.right_open = vsync_period / 2;
        app_hh_set.param.offset_3dsg.right_close = vsync_period - 1;
        BT_LOGI("Enabling 3DSG with LO:%d LC:%d RO:%d RC:%d",
                app_hh_set.param.offset_3dsg.left_open,
                app_hh_set.param.offset_3dsg.left_close,
                app_hh_set.param.offset_3dsg.right_open,
                app_hh_set.param.offset_3dsg.right_close);
    }
    else
    {
        app_hh_set.request = BSA_HH_3DSG_OFF_REQ;
        BT_LOGI("Disabling 3DSG");
    }
    status = BSA_HhSet(&app_hh_set);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("Failed to control 3DSG status:%d", status);
        return -1;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_hh_enable_3dsg_mode_manual
 **
 ** Description      Set 3DSG mode for one device
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_hh_enable_3dsg_mode_manual(void)
{
    tBSA_HH_SET app_hh_set;
    tBSA_HH_HANDLE handle;
    tBSA_STATUS status;
    UINT16 left_open;
    UINT16 left_close;
    UINT16 right_open;
    UINT16 right_close;

    BT_LOGI("Set 3DSG mode (manual Offsets/Delay)");
    handle = app_get_choice("Enter HID Handle");
    left_open = app_get_choice("Enter Left open");
    left_close = app_get_choice("Enter Left Close");
    right_open = app_get_choice("Enter Right Open");
    right_close = app_get_choice("Enter Right Close");

    BSA_HhSetInit(&app_hh_set);
    app_hh_set.handle = handle;
    app_hh_set.request = BSA_HH_3DSG_ON_REQ;
    app_hh_set.param.offset_3dsg.left_open = left_open;
    app_hh_set.param.offset_3dsg.left_close = left_close;
    app_hh_set.param.offset_3dsg.right_open = right_open;
    app_hh_set.param.offset_3dsg.right_close = right_close;

    BT_LOGI("Enabling 3DSG with LO:%d LC:%d RO:%d RC:%d",
            app_hh_set.param.offset_3dsg.left_open,
            app_hh_set.param.offset_3dsg.left_close,
            app_hh_set.param.offset_3dsg.right_open,
            app_hh_set.param.offset_3dsg.right_close);

    status = BSA_HhSet(&app_hh_set);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("Failed to Enable 3DSG status:%d", status);
        return -1;
    }
    return 0;
}


/*******************************************************************************
 **
 ** Function         app_hh_remove_dev
 **
 ** Description      Remove a device
 **
 ** Parameters
 **
 ** Returns          status
 **
 *******************************************************************************/
int app_hh_remove_dev(HidHost *hh, BD_ADDR bd_addr)
{
    //int dev_index;
    //int hh_dev_index;
    int status = 0;
    tBSA_HH_REMOVE_DEV hh_remove;
    //tBSA_SEC_REMOVE_DEV sec_remove;
    tAPP_HH_DEVICE *p_hh_dev;
    //tAPP_XML_REM_DEVICE *p_xml_dev;
#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
    tAPP_BLE_CLIENT_DB_ELEMENT *p_blecl_db_elmt;
#endif

    /* Example to Remove an HID Device (Server, Sec and HH Databases) */
    BT_LOGI("Remove HH Device");

    BT_CHECK_HANDLE(hh);
    p_hh_dev = app_hh_pdev_find_by_bdaddr(hh, bd_addr);

    if (p_hh_dev == NULL)
    {
        BT_LOGI("Bad HH Device");
        return -1;
    }

    /* Check hid device */
    if ((p_hh_dev->info_mask & APP_HH_DEV_USED) == 0)
    {
        BT_LOGI("HH Device not in use");
        return -1;
    }

    /* Remove this device from BSA's HH database */
    BSA_HhRemoveDevInit(&hh_remove);
    bdcpy(hh_remove.bd_addr, bd_addr);

    status = BSA_HhRemoveDev(&hh_remove);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_HhRemoveDev failed: %d",status);
        return -1;
    }

#if defined(APP_BTHID_INCLUDED) && (APP_BTHID_INCLUDED == TRUE)
    if (p_hh_dev->bthid_fd != -1)
    {
        app_bthid_close(p_hh_dev->bthid_fd);
        p_hh_dev->bthid_fd = -1;
    }
#endif
    /* Remove the device from HH application's memory */
    p_hh_dev->info_mask = 0;

    /* Remove the device from the HH application's database */
    app_hh_db_remove_by_bda(bd_addr);

    //rokidbt_mgr_sec_unpair(hh->caller, bd_addr);

#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
    BT_LOGI("Try to remove device from BLE DB");

    /* search BLE DB */
    p_blecl_db_elmt = app_ble_client_db_find_by_bda(bd_addr);
    if(p_blecl_db_elmt != NULL)
    {
        BT_LOGI("Device present in BLE DB, remove it.");
        app_ble_client_db_clear_by_bda(bd_addr);
        app_ble_client_db_save();
    }
    else
    {
        BT_LOGI("Device Not present in BLE DB");
    }
#endif

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_hh_send_set_prio
 **
 ** Description      Send set priority for the HID audio
 **
 ** Parameters       handle: HID handle of the device
 **                  bd_addr: BDADDR of RC
 **                  priority: priority value requested for channel
 **                  direction: direction of audio data
 **
 ** Returns          0 if success / -1 if error
 **
 *******************************************************************************/
int app_hh_send_set_prio(tBSA_HH_HANDLE handle, BD_ADDR bd_addr, UINT8 priority, UINT8 direction)
{
    int status = 0;
    tBSA_HH_SET app_hh_set;

    /* Send Set Priority Ext command for HID audio*/
    BT_LOGI("app_hh_send_set_prio priority:%d direction:%d", priority, direction);

    BSA_HhSetInit(&app_hh_set);
    app_hh_set.handle = handle;
    app_hh_set.request = BSA_HH_SET_PRIO_EXT_REQ;
    bdcpy(app_hh_set.param.prio.bd_addr, bd_addr);
    app_hh_set.param.prio.priority = priority;
    app_hh_set.param.prio.direction = direction;

    status = BSA_HhSet(&app_hh_set);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("Failed to set priority status:%d", status);
        return -1;
    }
    return 0;
}