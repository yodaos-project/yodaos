#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>

#include <bsa_rokid/bsa_api.h>
#include <bsa_rokid/uipc.h>
#include <bsa_rokid/btm_api.h>
#include "app_utils.h"
#include "app_xml_utils.h"
#include "ble_client.h"

#include "app_dm.h"
#include "app_manager.h"
#include "ble.h"

/*
 * Global Variables
 */
enum
{
    APP_BLECL_LOAD_ATTR_IDLE,
    APP_BLECL_LOAD_ATTR_START
};
#define APP_BLE_MAIN_DEFAULT_APPL_UUID    9000

BleClient *_ble_client;
static BOOLEAN service_search_pending = FALSE;

int app_ble_hogp_register_notification(BleClient *blec, int client);
int app_ble_hogp_enable_notification(BleClient *blec, int client);


static int app_ble_client_find_free_space(BleClient *blec)
{
    int index;

    for(index = 0; index < BSA_BLE_CLIENT_MAX ; index++)
    {
        if(!blec->ble_client[index].enabled)
        {
            return index;
        }
    }
    return -1;
}

static int app_ble_client_find_index_by_interface(BleClient *blec, tBSA_BLE_IF if_num)
{
    int index;

    for(index = 0; index < BSA_BLE_CLIENT_MAX ; index++)
    {
        if(blec->ble_client[index].client_if == if_num)
        {
            return index;
        }
    }
    return -1;
}

static int app_ble_client_find_index_by_conn_id(BleClient *blec, UINT16 conn_id)
{
    int index;

    for(index = 0; index < BSA_BLE_CLIENT_MAX ; index++)
    {
        if(blec->ble_client[index].conn_id == conn_id)
        {
            return index;
        }
    }
    return -1;
}

static int app_ble_client_find_conn_id_by_interface(BleClient *blec, tBSA_BLE_IF if_num)
{
    int index;

    for(index = 0; index < BSA_BLE_CLIENT_MAX ; index++)
    {
        if(blec->ble_client[index].client_if == if_num)
        {
            return blec->ble_client[index].conn_id;
        }
    }
    return -1;
}

static UINT16 app_ble_client_find_conn_id_by_index(BleClient *blec, int index)
{
    if ((index < 0) ||
        (index >= BSA_BLE_CLIENT_MAX))
    {
        return BSA_BLE_INVALID_CONN;
    }

    return blec->ble_client[index].conn_id;
}

static tAPP_BLE_CLIENT *app_ble_client_by_addr(BleClient *blec, BD_ADDR bd_addr)
{
    int index;

    for(index = 0; index < BSA_BLE_CLIENT_MAX ; index++)
    {
        if(blec->ble_client[index].enabled && bdcmp(blec->ble_client[index].server_addr , bd_addr)== 0)
        {
            return &blec->ble_client[index];
        }
    }
    return NULL;
}

#if PRINT_DEBUG
static void app_ble_print_info(char * fmt, ...)
{
    char buf[2000];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf,sizeof(buf)-2,fmt,ap);
    va_end(ap);
    strcat(buf, "\n");
    app_print_info(buf);
}


/*******************************************************************************
**
** Function         app_ble_client_handle_read_evt_ex
**
** Description      Function to handle callback for read events
**
** Returns          void
**
*******************************************************************************/
static void app_ble_client_handle_read_evt_ex(tBSA_BLE_CL_READ_MSG *cli_read, FP_DISP_FUNC pAppDispFunc)
{
    UINT8 value = 0;
    UINT16 temperature_measurement_interval = 0;
    UINT8 *pp;
    UINT16 lower_range = 0,upper_range = 0;

    switch(cli_read->char_id.uuid.uu.uuid16)
    {
    case BSA_BLE_GATT_UUID_SENSOR_LOCATION:
        pAppDispFunc("char_id:BSA_BLE_GATT_UUID_SENSOR_LOCATION");
        value = cli_read->value[0];
        if(value == 0)
            pAppDispFunc("Location : Other");
        else if(value == 1)
            pAppDispFunc("Location : Top of Shoe");
        else if(value == 2)
            pAppDispFunc("Location : In Shoe");
        else if(value == 3)
            pAppDispFunc("Location : Hip");
        else if(value == 4)
            pAppDispFunc("Location : Front Wheel");
        else if(value == 5)
            pAppDispFunc("Location : Left Crank");
        else if(value == 6)
            pAppDispFunc("Location : Right Crank");
        else if(value == 7)
            pAppDispFunc("Location : Left Pedal");
        else if(value == 8)
            pAppDispFunc("Location : Right Pedal");
        else if(value == 9)
            pAppDispFunc("Location : Front Hub");
        else if(value == 10)
            pAppDispFunc("Location : Real Dropout");
        else if(value == 11)
            pAppDispFunc("Location : Chainstay");
        else if(value == 12)
            pAppDispFunc("Location : Real Wheel");
        else if(value == 13)
            pAppDispFunc("Location : Real Hub");
        else if(value == 14)
            pAppDispFunc("Location : Chest");
        else
            pAppDispFunc("Location : Reserved");
        break;

    case BSA_BLE_GATT_UUID_CSC_FEATURE:
        pAppDispFunc("char_id:BSA_BLE_GATT_UUID_CSC_FEATURE");
        value = cli_read->value[0];
        if(value & 1)
            pAppDispFunc("Wheel Revolution Data Supported!");
        if(value & 2)
            pAppDispFunc("Crank Revolution Data Supported!");
        if(value & 4)
            pAppDispFunc("Multiple Sensor Location Supported!");
        break;

    case BSA_BLE_GATT_UUID_BLOOD_PRESSURE_FEATURE:
         pAppDispFunc("char_id: BSA_BLE_GATT_UUID_BLOOD_PRESSURE_FEATURE");
        value = cli_read->value[0];
        if(value & 1)
            pAppDispFunc("Body Movement Detection feature supported!");
        if(value & 2)
            pAppDispFunc("Cuff Fit Detection feature supported!");
        if(value & 4)
            pAppDispFunc("Irregular Pulse Detection feature supported!");
        if(value & 8)
            pAppDispFunc("Pulse Rate Range Detection feature supported!");
        if(value & 16)
            pAppDispFunc("Measurement Position Detection feature supported!");
        if(value & 32)
            pAppDispFunc("Multiple Bonds supported!");
         break;

    case BSA_BLE_GATT_UUID_RSC_FEATURE:
        pAppDispFunc("char_id:BSA_BLE_GATT_UUID_RSC_FEATURE");
        value = cli_read->value[0];
        if(value & 1)
            pAppDispFunc("Instantaneous Stride Length Measurement Supported!");
        if(value & 2)
            pAppDispFunc("Walking or Running Status Supported!");
        if(value & 4)
            pAppDispFunc("Calibration Procedure Supported!");
        if(value & 8)
            pAppDispFunc("Multiple Sensor Locations Supported!");
        break;

    case BSA_BLE_GATT_UUID_TEMPERATURE_TYPE:
        pAppDispFunc("char_id: BSA_BLE_GATT_UUID_TEMPERATURE_TYPE");
        value = cli_read->value[0];
        if(value == 1)
            pAppDispFunc("Location : Armpit");
        else if(value == 2)
            pAppDispFunc("Location : Body (general)");
        else if(value == 3)
            pAppDispFunc("Location : Ear (usually ear lobe)");
        else if(value == 4)
            pAppDispFunc("Location : Finger");
        else if(value == 5)
            pAppDispFunc("Location : Gastro-intestinal Tract");
        else if(value == 6)
            pAppDispFunc("Location : Mouth");
        else if(value == 7)
            pAppDispFunc("Location : Rectum");
        else if(value == 8)
            pAppDispFunc("Location : Toe");
        else if(value == 9)
            pAppDispFunc("Location : Tympanum (ear drum)");
        else
            pAppDispFunc("Location : Reserved");
        break;

    case BSA_BLE_GATT_UUID_TEMPERATURE_MEASUREMENT_INTERVAL:
        if(cli_read->descr_type.uuid.uu.uuid16 == BSA_BLE_GATT_UUID_CHAR_VALID_RANGE)
        {
            pAppDispFunc("char_id: BSA_BLE_GATT_UUID_CHAR_VALID_RANGE");
            pp = cli_read->value;
            STREAM_TO_UINT16(lower_range,pp);
            STREAM_TO_UINT16(upper_range,pp);
            pAppDispFunc("Lower Range:%d, Upper Range:%d",lower_range,upper_range);
        }
        else
        {
            pAppDispFunc("char_id: BSA_BLE_GATT_UUID_TEMPERATURE_MEASUREMENT_INTERVAL");
            pp = cli_read->value;
            STREAM_TO_UINT16(temperature_measurement_interval,pp);
            pAppDispFunc("Measurement Interval Seconds:%d, Minutes:%d, Hours:%d",temperature_measurement_interval,(temperature_measurement_interval/60),(temperature_measurement_interval/(60*60)));
        }
        break;

    case BSA_BLE_GATT_UUID_BATTERY_LEVEL:
        pAppDispFunc("char_id: BSA_BLE_GATT_UUID_BATTERY_LEVEL");
        value = cli_read->value[0];
        pAppDispFunc("Battery level %d percentage left", value);
        break;

    case BSA_BLE_GATT_UUID_MANU_NAME:
        pAppDispFunc("char_id: BSA_BLE_GATT_UUID_MANU_NAME");
        pAppDispFunc("Manufacturer name : %s", cli_read->value);
        break;

    case BSA_BLE_GATT_UUID_MODEL_NUMBER_STR :
        pAppDispFunc("char_id: BSA_BLE_GATT_UUID_MODEL_NUMBER_STR ");
        pAppDispFunc("Model Number : %s", cli_read->value);
        break;

    case BSA_BLE_GATT_UUID_SERIAL_NUMBER_STR :
        pAppDispFunc("char_id: BSA_BLE_GATT_UUID_SERIAL_NUMBER_STR ");
        pAppDispFunc("Model Number : %s", cli_read->value);
        break;

    case BSA_BLE_GATT_UUID_HW_VERSION_STR :
        pAppDispFunc("char_id: BSA_BLE_GATT_UUID_HW_VERSION_STR ");
        pAppDispFunc("Model Number : %s", cli_read->value);
        break;

    case BSA_BLE_GATT_UUID_FW_VERSION_STR:
        pAppDispFunc("char_id: BSA_BLE_GATT_UUID_FW_VERSION_STR ");
        pAppDispFunc("Model Number : %s", cli_read->value);
        break;

    case BSA_BLE_GATT_UUID_SW_VERSION_STR:
        pAppDispFunc("char_id: BSA_BLE_GATT_UUID_SW_VERSION_STR ");
        pAppDispFunc("Model Number : %s", cli_read->value);
        break;

    case BSA_BLE_GATT_UUID_SYSTEM_ID:
        pAppDispFunc("char_id: BSA_BLE_GATT_UUID_SYSTEM_ID ");
        pAppDispFunc("Model Number : %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
            cli_read->value[0], cli_read->value[1], cli_read->value[2], cli_read->value[3],
            cli_read->value[4], cli_read->value[5], cli_read->value[6], cli_read->value[7]);
        break;

    default:
        BT_LOGD("char_uuid:0x%04x", cli_read->char_id.uuid.uu.uuid16);
        BT_LOGD("descr_uuid:0x%04x", cli_read->descr_type.uuid.uu.uuid16);
        APP_DUMP("Read value", cli_read->value, cli_read->len);

        break;
    }
}

static void app_ble_client_handle_read_evt(tBSA_BLE_CL_READ_MSG *cli_read)
{
    app_ble_client_handle_read_evt_ex(cli_read, app_ble_print_info);
}

static void app_ble_client_handle_indication(tBSA_BLE_CL_NOTIF_MSG *cli_notif)
{
    UINT8 flag = 0;
    UINT8 *pp;
    BOOLEAN blp_bp_measurement_mmHg= FALSE;
    BOOLEAN blp_bp_measurement_kpa= FALSE;
    BOOLEAN blp_bp_measurement_timestamp= FALSE;
    BOOLEAN blp_bp_measurement_pulserate= FALSE;
    BOOLEAN blp_bp_measurement_userid= FALSE;
    BOOLEAN blp_bp_measurement_status= FALSE;
    UINT16 blp_comp_val_syst = 0;
    UINT16 blp_comp_val_Dias = 0;
    UINT16 blp_mean_arte_pressure = 0;
    UINT8 blp_measurement_timestamp = 0;
    UINT16 blp_measurement_pulserate = 0;
    UINT8 blp_measurement_user_id = 0;
    UINT16 blp_measurement_status = 0;
    BOOLEAN htp_tmp_measurement_fah = FALSE;
    BOOLEAN htp_tmp_measurement_cels = FALSE;
    BOOLEAN htp_tmp_measurement_timestamp = FALSE;
    BOOLEAN htp_tmp_type_flag = FALSE;
    UINT32 tmp_measurement_value_cels = 0;
    UINT32 tmp_measurement_value_fah = 0;
    UINT16 year = 0;
    UINT8 month = 0;
    UINT8 day = 0;
    UINT8 hours = 0;
    UINT8 minutes = 0;
    UINT8 seconds = 0;
    UINT8 value = 0;
    UINT8 float_exponent_val = 0;
    float f1oat_temp_conversion = 0;

    switch(cli_notif->char_id.char_id.uuid.uu.uuid16)
    {
    case BSA_BLE_GATT_UUID_BLOOD_PRESSURE_MEASUREMENT:
        BT_LOGI("BSA_BLE_GATT_UUID_BLOOD_PRESSURE_MEASUREMENT, cli_notif->len %d", cli_notif->len );
        if(cli_notif->len <= BSA_BLE_GATT_LENGTH_OF_BLOOD_PRESSURE_MEASUREMENT)
        {
            pp = cli_notif->value;
            STREAM_TO_UINT8(flag, pp);
            BT_LOGI("Flags:0x%x", flag);
            if(flag & 0x1)
            {
                blp_bp_measurement_kpa = TRUE;
                BT_LOGI("Blood pressure for Systolic, Diastolic and MAP in units of kPa");
            }
            else
            {
                blp_bp_measurement_mmHg = TRUE;
                BT_LOGI("Blood pressure for Systolic, Diastolic and MAP in units of mmHg");
            }
            if((flag >> 1) & 0x1)
            {
                blp_bp_measurement_timestamp = TRUE;
                BT_LOGI("Time Stamp present");
            }
            else
            {
                BT_LOGI("Time Stamp not present");
            }
            if((flag >> 2) & 0x1)
            {
                blp_bp_measurement_pulserate = TRUE;
                BT_LOGI("Pulse Rate present");
            }
            else
            {
                BT_LOGI("Pulse Rate not present");
            }
            if((flag >> 3) & 0x1)
            {
                blp_bp_measurement_userid = TRUE;
                BT_LOGI("User ID present");
            }
            else
            {
                BT_LOGI("User ID not present");
            }
            if((flag >> 4) & 0x1)
            {
                blp_bp_measurement_status = TRUE;
                BT_LOGI("Measurement Status present");
            }
            else
            {
                BT_LOGI("Measurement Status not present");
            }

            if(blp_bp_measurement_mmHg)
            {
                STREAM_TO_UINT16(blp_comp_val_syst, pp);
                BT_LOGI("Blood Pressure Measurement Compound Value - Systolic (mmHg):%d", blp_comp_val_syst);
                STREAM_TO_UINT16(blp_comp_val_Dias, pp);
                BT_LOGI("Blood Pressure Measurement Compound Value - Diastolic (mmHg):%d", blp_comp_val_Dias);
                STREAM_TO_UINT16(blp_mean_arte_pressure, pp);
                BT_LOGI("Blood Pressure Measurement Compound Value - Mean Arterial Pressure (mmHg):%d", blp_mean_arte_pressure);
            }

            if(blp_bp_measurement_kpa)
            {
                STREAM_TO_UINT16(blp_comp_val_syst, pp);
                BT_LOGI("Blood Pressure Measurement Compound Value - Systolic (kPa):%d", blp_comp_val_syst);
                STREAM_TO_UINT16(blp_comp_val_Dias, pp);
                BT_LOGI("Blood Pressure Measurement Compound Value - Diastolic (kPa):%d", blp_comp_val_Dias);
                STREAM_TO_UINT16(blp_mean_arte_pressure, pp);
                BT_LOGI("Blood Pressure Measurement Compound Value - Mean Arterial Pressure (kPa):%d", blp_mean_arte_pressure);
            }

            if(blp_bp_measurement_timestamp)
            {
                STREAM_TO_UINT8(blp_measurement_timestamp, pp);
                BT_LOGI("Time Stamp:%d",blp_measurement_timestamp);
            }

            if(blp_bp_measurement_pulserate)
            {
                STREAM_TO_UINT16(blp_measurement_pulserate, pp);
                BT_LOGI("Pulse Rate:%d",blp_measurement_pulserate);
            }

            if(blp_bp_measurement_userid)
            {
                STREAM_TO_UINT8(blp_measurement_user_id, pp);
                BT_LOGI("User Id:%d",blp_measurement_user_id);
            }

            if(blp_bp_measurement_status)
            {
                STREAM_TO_UINT16(blp_measurement_status, pp);
                BT_LOGI("Measurement Status:%d",blp_measurement_status);
            }
        }
        else
        {
            BT_LOGE("Wrong length:%d", cli_notif->len);
        }
        break;

    case BSA_BLE_GATT_UUID_TEMPERATURE_MEASUREMENT:
        BT_LOGI("BSA_BLE_GATT_UUID_TEMPERATURE_MEASUREMENT, cli_notif->len %d", cli_notif->len );
        pp = cli_notif->value;
        STREAM_TO_UINT8(flag, pp);
        BT_LOGI("Flags:0x%x", flag);
        if(flag & 0x1)
        {
            htp_tmp_measurement_fah = TRUE;
            BT_LOGI("Temperature Measurement Value in units of Fahrenheit");
        }
        else
        {
            htp_tmp_measurement_cels = TRUE;
            BT_LOGI("Temperature Measurement Value in units of Celsius");
        }

        if((flag >> 1) & 0x1)
        {
            htp_tmp_measurement_timestamp = TRUE;
            BT_LOGI("Time Stamp field present");
        }
        else
        {
            BT_LOGI("Time Stamp field not present");
        }

        if((flag >> 2) & 0x1)
        {
            htp_tmp_type_flag = TRUE;
            BT_LOGI("Temperature Type field present");
        }
        else
        {
            BT_LOGI("Temperature Type field not present");
        }

        if(htp_tmp_measurement_cels)
        {
            STREAM_TO_UINT24(tmp_measurement_value_cels, pp);
            STREAM_TO_UINT8(float_exponent_val,pp);

            if (float_exponent_val==0xFF)
            {
                 f1oat_temp_conversion = (float)tmp_measurement_value_cels/10.0;
                 BT_LOGI("Temperature Measurement Value (Celsius):%.1f", f1oat_temp_conversion);
            }
            else
            {
                 BT_LOGI("Temperature Measurement Value (Celsius):%d", tmp_measurement_value_cels);
            }
        }

        if(htp_tmp_measurement_fah)
        {
            STREAM_TO_UINT24(tmp_measurement_value_fah, pp);
            STREAM_TO_UINT8(float_exponent_val,pp);

            if (float_exponent_val==0xFF)
            {
                 f1oat_temp_conversion = (float)tmp_measurement_value_fah/10.0;
                 BT_LOGI("Temperature Measurement Value (Fahrenheit):%.1f", f1oat_temp_conversion);
            }
            else
            {
                BT_LOGI("Temperature Measurement Value (Fahrenheit):%d", tmp_measurement_value_fah);
            }
        }

        if(htp_tmp_measurement_timestamp)
        {
            STREAM_TO_UINT16(year, pp);
            STREAM_TO_UINT8(month, pp);
            STREAM_TO_UINT8(day, pp);
            STREAM_TO_UINT8(hours, pp);
            STREAM_TO_UINT8(minutes, pp);
            STREAM_TO_UINT8(seconds, pp);
            BT_LOGI("Temperature Measurement Time stamp Date: Year-%d, Month-%d, Day-%d", year, month, day);
            BT_LOGI("Time: Hours-%d, Minutes-%d, Seconds-%d", hours, minutes, seconds);
        }
        if(htp_tmp_type_flag)
        {
            STREAM_TO_UINT8(value, pp);
            if(value == 1)
                BT_LOGI("Location : Armpit");
            else if(value == 2)
                BT_LOGI("Location : Body (general)");
            else if(value == 3)
                BT_LOGI("Location : Ear (usually ear lobe)");
            else if(value == 4)
                BT_LOGI("Location : Finger");
            else if(value == 5)
                BT_LOGI("Location : Gastro-intestinal Tract");
            else if(value == 6)
                BT_LOGI("Location : Mouth");
            else if(value == 7)
                BT_LOGI("Location : Rectum");
            else if(value == 8)
                BT_LOGI("Location : Toe");
            else if(value == 9)
                BT_LOGI("Location : Tympanum (ear drum)");
            else
                BT_LOGI("Location : Reserved");
        }
            break;

        default:
                break;
    }
}


/*******************************************************************************
**
** Function         app_ble_client_handle_notification_ex
**
** Description      Function to handle callback for notification events
**
** Returns          void
**
*******************************************************************************/
static void app_ble_client_handle_notification_ex(tBSA_BLE_CL_NOTIF_MSG *cli_notif,FP_DISP_FUNC pAppDispFunc)
{
    UINT8 flag = 0;
    UINT8 *pp;
    UINT32 cwr = 0;
    UINT16 lwet = 0;
    UINT16 ccr = 0;
    UINT16 lcet = 0;
    BOOLEAN blp_bp_measurement_mmHg= FALSE;
    BOOLEAN blp_bp_measurement_kpa= FALSE;
    BOOLEAN blp_bp_measurement_timestamp= FALSE;
    BOOLEAN blp_bp_measurement_pulserate= FALSE;
    BOOLEAN blp_bp_measurement_userid= FALSE;
    BOOLEAN blp_bp_measurement_status= FALSE;
    UINT16 blp_comp_val_syst = 0;
    UINT16 blp_comp_val_Dias = 0;
    UINT16 blp_mean_arte_pressure = 0;
    UINT8 blp_measurement_timestamp = 0;
    UINT16 blp_measurement_pulserate = 0;
    UINT8 blp_measurement_user_id = 0;
    UINT16 blp_measurement_status = 0;
    UINT16 rscs_is = 0;
    UINT8 rscs_ic = 0;
    UINT16 rscs_isl = 0;
    UINT32 rscs_td = 0;
    BOOLEAN htp_tmp_measurement_fah = FALSE;
    BOOLEAN htp_tmp_measurement_cels = FALSE;
    BOOLEAN htp_tmp_measurement_timestamp = FALSE;
    BOOLEAN htp_tmp_type_flag = FALSE;
    UINT32 tmp_measurement_value_cels = 0;
    UINT32 tmp_measurement_value_fah = 0;
    UINT16 year = 0;
    UINT8 month = 0;
    UINT8 day = 0;
    UINT8 hours = 0;
    UINT8 minutes = 0;
    UINT8 seconds = 0;
    UINT8 value = 0;
    UINT8 float_exponent_val = 0;
    float f1oat_temp_conversion = 0;



    switch(cli_notif->char_id.char_id.uuid.uu.uuid16)
    {
    case BSA_BLE_GATT_UUID_CSC_MEASUREMENT:
        if(cli_notif->len == BSA_BLE_GATT_LENGTH_OF_CSC_MEASUREMENT)
        {
            pp = cli_notif->value;
            STREAM_TO_UINT8(flag, pp);
            pAppDispFunc("Flags:0x%x", flag);
            STREAM_TO_UINT32(cwr, pp);
            pAppDispFunc("Cumulative Wheel Revolution:%d", cwr);
            STREAM_TO_UINT16(lwet, pp);
            pAppDispFunc("Last Wheel Event Time:%d", lwet);
            STREAM_TO_UINT16(ccr, pp);
            pAppDispFunc("Cumulative Crank Revolution:%d", ccr);
            STREAM_TO_UINT16(lcet, pp);
            pAppDispFunc("Last Crank Event Time: %d", lcet);
        }
        else
        {
            BT_LOGE("Wrong length:%d", cli_notif->len);
        }
        break;

    case BSA_BLE_GATT_UUID_INTERMEDIATE_CUFF_PRESSURE:
        pAppDispFunc("BSA_BLE_GATT_UUID_INTERMEDIATE_CUFF_PRESSURE, cli_notif->len %d", cli_notif->len );
        if(cli_notif->len <= BSA_BLE_GATT_LENGTH_OF_BLOOD_PRESSURE_MEASUREMENT)
        {
            pp = cli_notif->value;
            STREAM_TO_UINT8(flag, pp);
            pAppDispFunc("Flags:0x%x", flag);
            if(flag & 0x1)
            {
                blp_bp_measurement_kpa = TRUE;
                pAppDispFunc("Blood pressure for Systolic, Diastolic and MAP in units of kPa");
            }
            else
            {
                blp_bp_measurement_mmHg = TRUE;
                pAppDispFunc("Blood pressure for Systolic, Diastolic and MAP in units of mmHg");
            }
            if((flag >> 1) & 0x1)
            {
                blp_bp_measurement_timestamp = TRUE;
                pAppDispFunc("Time Stamp present");
            }
            else
            {
                pAppDispFunc("Time Stamp not present");
            }
            if((flag >> 2) & 0x1)
            {
                blp_bp_measurement_pulserate = TRUE;
                pAppDispFunc("Pulse Rate present");
            }
            else
            {
                pAppDispFunc("Pulse Rate not present");
            }
            if((flag >> 3) & 0x1)
            {
                blp_bp_measurement_userid = TRUE;
                pAppDispFunc("User ID present");
            }
            else
            {
                pAppDispFunc("User ID not present");
            }
            if((flag >> 4) & 0x1)
            {
                blp_bp_measurement_status = TRUE;
                pAppDispFunc("Measurement Status present");
            }
            else
            {
                pAppDispFunc("Measurement Status not present");
            }

            if(blp_bp_measurement_mmHg)
            {
                STREAM_TO_UINT16(blp_comp_val_syst, pp);
                pAppDispFunc("Blood Pressure Measurement Compound Value - Systolic (mmHg):%d", blp_comp_val_syst);
                STREAM_TO_UINT16(blp_comp_val_Dias, pp);
                pAppDispFunc("Blood Pressure Measurement Compound Value - Diastolic (mmHg):%d", blp_comp_val_Dias);
                STREAM_TO_UINT16(blp_mean_arte_pressure, pp);
                pAppDispFunc("Blood Pressure Measurement Compound Value - Mean Arterial Pressure (mmHg):%d", blp_mean_arte_pressure);
            }

            if(blp_bp_measurement_kpa)
            {
                STREAM_TO_UINT16(blp_comp_val_syst, pp);
                pAppDispFunc("Blood Pressure Measurement Compound Value - Systolic (kPa):%d", blp_comp_val_syst);
                STREAM_TO_UINT16(blp_comp_val_Dias, pp);
                pAppDispFunc("Blood Pressure Measurement Compound Value - Diastolic (kPa):%d", blp_comp_val_Dias);
                STREAM_TO_UINT16(blp_mean_arte_pressure, pp);
                pAppDispFunc("Blood Pressure Measurement Compound Value - Mean Arterial Pressure (kPa):%d", blp_mean_arte_pressure);
            }

            if(blp_bp_measurement_timestamp)
            {
                STREAM_TO_UINT8(blp_measurement_timestamp, pp);
                pAppDispFunc("Time Stamp:%d",blp_measurement_timestamp);
            }

            if(blp_bp_measurement_pulserate)
            {
                STREAM_TO_UINT16(blp_measurement_pulserate, pp);
                pAppDispFunc("Pulse Rate:%d",blp_measurement_pulserate);
            }

            if(blp_bp_measurement_userid)
            {
                STREAM_TO_UINT8(blp_measurement_user_id, pp);
                pAppDispFunc("User Id:%d",blp_measurement_user_id);
            }

            if(blp_bp_measurement_status)
            {
                STREAM_TO_UINT16(blp_measurement_status, pp);
                pAppDispFunc("Measurement Status:%d",blp_measurement_status);
            }
        }
        else
        {
            BT_LOGE("Wrong length:%d", cli_notif->len);
        }
    break;

    case BSA_BLE_GATT_UUID_RSC_MEASUREMENT:
        if(cli_notif->len == BSA_BLE_GATT_LENGTH_OF_RSC_MEASUREMENT)
        {
            pp = cli_notif->value;
            STREAM_TO_UINT8(flag, pp);
            pAppDispFunc("Flags:0x%x", flag);
            STREAM_TO_UINT16(rscs_is, pp);
            pAppDispFunc("Instantaneous Speed:%d", rscs_is);
            STREAM_TO_UINT8(rscs_ic, pp);
            pAppDispFunc("Instantaneous Cadence:%d", rscs_ic);
            STREAM_TO_UINT16(rscs_isl, pp);
            pAppDispFunc("Instantaneous Stride Length:%d", rscs_isl);
            STREAM_TO_UINT32(rscs_td, pp);
            pAppDispFunc("Total Distance: %d", rscs_td);
        }
        else
        {
            BT_LOGE("Wrong length:%d", cli_notif->len);
        }
        break;

     case BSA_BLE_GATT_UUID_INTERMEDIATE_TEMPERATURE:
        pAppDispFunc("BSA_BLE_GATT_UUID_INTERMEDIATE_TEMPERATURE, cli_notif->len %d", cli_notif->len );
        pp = cli_notif->value;
        STREAM_TO_UINT8(flag, pp);
        pAppDispFunc("Flags:0x%x", flag);
        if(flag & 0x1)
        {
            htp_tmp_measurement_fah = TRUE;
            pAppDispFunc("Temperature Measurement Value in units of Fahrenheit");
        }
        else
        {
            htp_tmp_measurement_cels = TRUE;
            pAppDispFunc("Temperature Measurement Value in units of Celsius");
        }

        if((flag >> 1) & 0x1)
        {
            htp_tmp_measurement_timestamp = TRUE;
            pAppDispFunc("Time Stamp field present");
        }
        else
        {
            pAppDispFunc("Time Stamp field not present");
        }

        if((flag >> 2) & 0x1)
        {
            htp_tmp_type_flag = TRUE;
            pAppDispFunc("Temperature Type field present");
        }
        else
        {
            pAppDispFunc("Temperature Type field not present");
        }

        if(htp_tmp_measurement_cels)
        {
            STREAM_TO_UINT24(tmp_measurement_value_cels, pp);
            STREAM_TO_UINT8(float_exponent_val,pp);
            if (float_exponent_val==0xFF)
            {
                 f1oat_temp_conversion = (float)tmp_measurement_value_cels/10.0;
                 pAppDispFunc("Temperature Measurement Value (Celsius):%.1f", f1oat_temp_conversion);
            }
            else
            {
                 pAppDispFunc("Temperature Measurement Value (Celsius):%d", tmp_measurement_value_cels);
            }
        }

        if(htp_tmp_measurement_fah)
        {
            STREAM_TO_UINT24(tmp_measurement_value_fah, pp);
            STREAM_TO_UINT8(float_exponent_val,pp);
            if (float_exponent_val==0xFF)
            {
                 f1oat_temp_conversion = (float)tmp_measurement_value_fah/10.0;
                 pAppDispFunc("Temperature Measurement Value (Fahrenheit):%.1f", f1oat_temp_conversion);
            }
            else
            {
                pAppDispFunc("Temperature Measurement Value (Fahrenheit):%d", tmp_measurement_value_fah);
            }
        }

        if(htp_tmp_measurement_timestamp)
        {
            STREAM_TO_UINT16(year, pp);
            STREAM_TO_UINT8(month, pp);
            STREAM_TO_UINT8(day, pp);
            STREAM_TO_UINT8(hours, pp);
            STREAM_TO_UINT8(minutes, pp);
            STREAM_TO_UINT8(seconds, pp);
            pAppDispFunc("Temperature Measurement Time stamp Date: Year-%d, Month-%d, Day-%d", year, month, day);
            pAppDispFunc("Time: Hours-%d, Minutes-%d, Seconds-%d", hours, minutes, seconds);
        }
        if(htp_tmp_type_flag)
        {
            STREAM_TO_UINT8(value, pp);
            if(value == 1)
                pAppDispFunc("Location : Armpit");
            else if(value == 2)
                pAppDispFunc("Location : Body (general)");
            else if(value == 3)
                pAppDispFunc("Location : Ear (usually ear lobe)");
            else if(value == 4)
                pAppDispFunc("Location : Finger");
            else if(value == 5)
                pAppDispFunc("Location : Gastro-intestinal Tract");
            else if(value == 6)
                pAppDispFunc("Location : Mouth");
            else if(value == 7)
                pAppDispFunc("Location : Rectum");
            else if(value == 8)
                pAppDispFunc("Location : Toe");
            else if(value == 9)
                pAppDispFunc("Location : Tympanum (ear drum)");
            else
                pAppDispFunc("Location : Reserved");
         }
        break;


    default:
        break;
    }
}

static void app_ble_client_handle_notification(tBSA_BLE_CL_NOTIF_MSG *cli_notif)
{
    app_ble_client_handle_notification_ex(cli_notif,app_print_info);
}
#endif

static int number_of_load_attr = 0;
static int state_of_load_attr = APP_BLECL_LOAD_ATTR_IDLE;
/*******************************************************************************
 **
 ** Function        app_ble_client_load_attr
 **
 ** Description     This is the cache load function
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
static int app_ble_client_load_attr(tAPP_BLE_CLIENT_DB_ELEMENT *p_blecl_db_elmt,
                             BD_ADDR bd_addr, UINT16 conn_id)
{
    int index = 0;
    int skip_count = 0;
    int sent = 0;
    tBSA_STATUS status;

    tBSA_BLE_CL_CACHE_LOAD cache_load;
    tAPP_BLE_CLIENT_DB_ATTR *cur_attr;

    BSA_BleClCacheLoadInit(&cache_load);

    /* in case of new device */
    if(p_blecl_db_elmt == NULL)
    {
        bdcpy(cache_load.bd_addr, bd_addr);
        cache_load.conn_id = conn_id;
        cache_load.num_attr = 0;
        cache_load.more = 0;
        status = BSA_BleClCacheLoad(&cache_load);
        if(status != BSA_SUCCESS)
        {
            BT_LOGE("BSA_BleClCacheLoad failed status = %d", status);
            return -1;
        }
        state_of_load_attr = APP_BLECL_LOAD_ATTR_IDLE;
        number_of_load_attr = 0;
        return 1;
    }

    cur_attr = p_blecl_db_elmt->p_attr;

    if((number_of_load_attr == 0) && (state_of_load_attr == APP_BLECL_LOAD_ATTR_IDLE)) /* first load attribute */
    {
        state_of_load_attr = APP_BLECL_LOAD_ATTR_START;
        if(p_blecl_db_elmt->p_attr == NULL) /* no attribute in cache */
        {
            bdcpy(cache_load.bd_addr, bd_addr);
            cache_load.conn_id = conn_id;
            cache_load.num_attr = 0;
            cache_load.more = 0;
            status = BSA_BleClCacheLoad(&cache_load);
            if(status != BSA_SUCCESS)
            {
                BT_LOGE("BSA_BleClCacheLoad failed status = %d", status);
                return -1;
            }
            state_of_load_attr = APP_BLECL_LOAD_ATTR_IDLE;
            number_of_load_attr = 0;
            sent = 1;
        }
        else /* Cache has attribute list */
        {
            while((cur_attr != NULL) && !(sent))
            {
                if(index == BSA_BLE_CL_NV_LOAD_MAX)
                {
                    bdcpy(cache_load.bd_addr, bd_addr);
                    cache_load.conn_id = conn_id;
                    cache_load.num_attr = index;
                    if(cur_attr)
                    {
                        cache_load.more = 1;
                        state_of_load_attr = APP_BLECL_LOAD_ATTR_START;
                        number_of_load_attr = index;
                    }
                    status = BSA_BleClCacheLoad(&cache_load);
                    if(status != BSA_SUCCESS)
                    {
                        BT_LOGE("BSA_BleClCacheLoad failed status = %d", status);
                        return -1;
                    }
                    sent = 1;
                }
                else
                {
                    cache_load.attr[index].uuid = cur_attr->attr_UUID;
                    cache_load.attr[index].s_handle = cur_attr->handle;
                    cache_load.attr[index].attr_type = cur_attr->attr_type;
                    cache_load.attr[index].id = cur_attr->id;
                    cache_load.attr[index].prop = cur_attr->prop;
                    cache_load.attr[index].is_primary = cur_attr->is_primary;
                    BT_LOGD("\t Attr[0x%04x] handle[0x%04x] uuid[0x%04x] inst[%d] type[%d] prop[0x%1x] is_primary[%d]",
                              index, cache_load.attr[index].s_handle,
                              cache_load.attr[index].uuid.uu.uuid16,
                              cache_load.attr[index].id,
                              cache_load.attr[index].attr_type,
                              cache_load.attr[index].prop,
                              cache_load.attr[index].is_primary);
                    index++;
                    cur_attr = cur_attr->next;
                }
            }
            if(index && !(sent))
            {
                bdcpy(cache_load.bd_addr, bd_addr);
                cache_load.conn_id = conn_id;
                cache_load.num_attr = index+1;
                cache_load.more = 0;
                status = BSA_BleClCacheLoad(&cache_load);
                if(status != BSA_SUCCESS)
                {
                    BT_LOGE("BSA_BleClCacheLoad failed status = %d", status);
                    return -1;
                }
                state_of_load_attr = APP_BLECL_LOAD_ATTR_IDLE;
                number_of_load_attr = 0;
                sent = 1;
            }
        }
    }
    /* Need to send pending attributes to BSA server */
    else if((number_of_load_attr != 0) && (state_of_load_attr == APP_BLECL_LOAD_ATTR_START))
    {
        while(skip_count < (number_of_load_attr))/* skip attributes which was sent already */
        {
            cur_attr = cur_attr->next;
            skip_count++;
        }
        while((cur_attr != NULL) && (!sent))
        {
            if(index == BSA_BLE_CL_NV_LOAD_MAX)
            {
                bdcpy(cache_load.bd_addr, bd_addr);
                cache_load.conn_id = conn_id;
                cache_load.num_attr = index;
                if(cur_attr)
                {
                    cache_load.more = 1;
                    state_of_load_attr = APP_BLECL_LOAD_ATTR_START;
                    number_of_load_attr = number_of_load_attr + index;
                }
                status = BSA_BleClCacheLoad(&cache_load);
                if(status != BSA_SUCCESS)
                {
                    BT_LOGE("BSA_BleClCacheLoad failed status = %d", status);
                    return -1;
                }
                sent = 1;
            }
            else
            {
                cache_load.attr[index].uuid = cur_attr->attr_UUID;
                cache_load.attr[index].s_handle = cur_attr->handle;
                cache_load.attr[index].attr_type = cur_attr->attr_type;
                cache_load.attr[index].id = cur_attr->id;
                cache_load.attr[index].prop = cur_attr->prop;
                cache_load.attr[index].is_primary = cur_attr->is_primary;
                BT_LOGD("\t Attr[0x%04x] handle[%d] uuid[0x%04x] inst[%d] type[%d] prop[0x%1x] is_primary[%d]",
                              index, cache_load.attr[index].s_handle,
                              cache_load.attr[index].uuid.uu.uuid16,
                              cache_load.attr[index].id,
                              cache_load.attr[index].attr_type,
                              cache_load.attr[index].prop,
                              cache_load.attr[index].is_primary);
                index++;
                cur_attr = cur_attr->next;
            }
        }
        if(index && !(sent))
        {
            bdcpy(cache_load.bd_addr, bd_addr);
            cache_load.conn_id = conn_id;
            cache_load.num_attr = index;
            cache_load.more = 0;
            status = BSA_BleClCacheLoad(&cache_load);
            if(status != BSA_SUCCESS)
            {
                BT_LOGE("BSA_BleClCacheLoad failed status = %d", status);
                return -1;
            }
            state_of_load_attr = APP_BLECL_LOAD_ATTR_IDLE;
            number_of_load_attr = 0;
            sent = 1;
        }

    }
    else
    {
        BT_LOGE("Abnormal situation number_of_load_attr:%d, state_of_load_attr:%d",
                    number_of_load_attr, state_of_load_attr);
    }
    return 1;
}

static void app_ble_client_profile_cback(tBSA_BLE_EVT event,  tBSA_BLE_MSG *p_data)
{
    int index;
    int client_num;
    int status;
    tAPP_BLE_CLIENT_DB_ELEMENT *p_blecl_db_elmt;
    tAPP_BLE_CLIENT_DB_ATTR *p_blecl_db_attr;
    tBSA_BLE_CL_INDCONF ind_conf;
    tBSA_BLE_CL_CFG_MTU cfg_mtu;
    BleClient *blec = _ble_client;
    RKBluetooth *bt = blec->caller;
    BT_BLE_MSG msg = {0};

    switch (event)
    {
        case BSA_BLE_CL_DEREGISTER_EVT:
            BT_LOGI("BSA_BLE_CL_DEREG_EVT cif=%d status=%d", p_data->cli_deregister.client_if,
                              p_data->cli_deregister.status );
            client_num = app_ble_client_find_index_by_interface(blec, p_data->cli_deregister.client_if);
            if(client_num < 0)
            {
                BT_LOGE("No client!!");
                break;
            }
            blec->ble_client[client_num].enabled = FALSE;
            blec->ble_client[client_num].client_if = BSA_BLE_INVALID_IF;
            blec->ble_client[client_num].conn_id = BSA_BLE_INVALID_CONN;
            break;

        case BSA_BLE_CL_OPEN_EVT:
            BT_LOGI("BSA_BLE_CL_OPEN_EVT client_if=%d conn_id=%d status=%d",
                              p_data->cli_open.client_if,
                              p_data->cli_open.conn_id,
                              p_data->cli_open.status);

            BTDevice dev = {0};
            bdcpy(dev.bd_addr, p_data->cli_open.bd_addr);
            if (rokidbt_find_scaned_device(blec->caller, &dev) == 1) {
                snprintf((char *)msg.cli_open.name, sizeof(msg.cli_open.name), "%s", dev.name);
            } else if (rokidbt_find_bonded_device(blec->caller, &dev) == 1) {
                snprintf((char *)msg.cli_open.name, sizeof(msg.cli_open.name), "%s", dev.name);
            }
            if (p_data->cli_open.status == BSA_SUCCESS)
            {
                client_num = app_ble_client_find_index_by_interface(blec, p_data->cli_open.client_if);
                if(client_num < 0)
                {
                    BT_LOGE("No client!!");
                    break;
                }
                blec->ble_client[client_num].conn_id = p_data->cli_open.conn_id;
                bdcpy(blec->ble_client[client_num].server_addr, p_data->cli_open.bd_addr);

                /* XML Database update */
                app_read_xml_remote_devices();
                /* Add BLE service for this devices in XML database */
                app_xml_add_trusted_services_db(app_xml_remote_devices_db,
                        APP_NUM_ELEMENTS(app_xml_remote_devices_db), p_data->cli_open.bd_addr,
                        BSA_BLE_SERVICE_MASK);
                /* Check if the name in the inquiry responses database */
                for (index = 0; index < BT_DISC_NB_DEVICES; index++)
                {
                    if (!bdcmp(bt->discovery_devs[index].device.bd_addr, p_data->cli_open.bd_addr))
                    {
                        app_xml_update_name_db(app_xml_remote_devices_db,
                                APP_NUM_ELEMENTS(app_xml_remote_devices_db), p_data->cli_open.bd_addr,
                                bt->discovery_devs[index].device.name);
#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE) /* BLE Connection from DB */
                        app_xml_update_ble_addr_type_db(app_xml_remote_devices_db,
                                APP_NUM_ELEMENTS(app_xml_remote_devices_db), p_data->cli_open.bd_addr,
                                bt->discovery_devs[index].device.ble_addr_type);
                        app_xml_update_device_type_db(app_xml_remote_devices_db,
                                APP_NUM_ELEMENTS(app_xml_remote_devices_db), p_data->cli_open.bd_addr,
                                bt->discovery_devs[index].device.device_type);
                        app_xml_update_inq_result_type_db(app_xml_remote_devices_db,
                                APP_NUM_ELEMENTS(app_xml_remote_devices_db), p_data->cli_open.bd_addr,
                                bt->discovery_devs[index].device.inq_result_type);
#endif
                        break;
                    }
                }
                snprintf((char *)blec->ble_client[client_num].name, sizeof(blec->ble_client[client_num].name), "%s", dev.name);
                status = app_write_xml_remote_devices();
                if (status < 0)
                {
                    BT_LOGE("app_ble_write_remote_devices failed: %d", status);
                }
                /* check DB first */
                p_blecl_db_elmt = app_ble_client_db_find_by_bda(p_data->cli_open.bd_addr);
                if(p_blecl_db_elmt == NULL) /* this is new device */
                {
                    BT_LOGI("New device. Update BLE DB!!");
                    /* update BLE DB */
                    p_blecl_db_elmt = app_ble_client_db_alloc_element();
                    if(p_blecl_db_elmt != NULL)
                    {
                        bdcpy(p_blecl_db_elmt->bd_addr, p_data->cli_open.bd_addr);
                        p_blecl_db_elmt->app_handle = p_data->cli_open.client_if;
                        app_ble_client_db_add_element(p_blecl_db_elmt);
                        BT_LOGD("save BDA:%02X:%02X:%02X:%02X:%02X:%02X, client_if= %d ",
                              p_blecl_db_elmt->bd_addr[0], p_blecl_db_elmt->bd_addr[1],
                              p_blecl_db_elmt->bd_addr[2], p_blecl_db_elmt->bd_addr[3],
                              p_blecl_db_elmt->bd_addr[4], p_blecl_db_elmt->bd_addr[5],
                              p_data->cli_open.client_if);
                        app_ble_client_db_save();
                    }
                }
                else
                {
                    BT_LOGI("already exist in BLE DB!!");
                }

                BSA_BleClCfgMtuInit(&cfg_mtu);
                cfg_mtu.conn_id = p_data->cli_open.conn_id;
                cfg_mtu.mtu = BSA_BLE_MAX_PDU_LENGTH;
                BSA_BleClCfgMtu(&cfg_mtu);
            }
            if (blec->listener) {
                msg.cli_open.status = p_data->cli_open.status;
                bdcpy(msg.cli_open.bd_addr, p_data->cli_open.bd_addr);
                blec->listener(blec->listener_handle, BT_BLE_CL_OPEN_EVT, &msg);
            }
            break;

        case BSA_BLE_CL_SEARCH_RES_EVT:
            BT_LOGI("BSA_BLE_CL_SEARCH_RES_EVT: conn_id = %d",
                    p_data->cli_search_res.conn_id);
            /* always assume 16 bits UUID */
            //BT_LOGI(" Searched Service name:%s(uuid:0x%04x)",
            //      app_ble_display_service_name(p_data->cli_search_res.service_uuid.id.uuid.uu.uuid16),
            //        p_data->cli_search_res.service_uuid.id.uuid.uu.uuid16);

            service_search_pending = FALSE;
            break;

        case BSA_BLE_CL_SEARCH_CMPL_EVT:
            BT_LOGI("BSA_BLE_CL_SEARCH_CMPL_EVT = %d",
                    p_data->cli_search_cmpl.status);
            if (service_search_pending == TRUE)
            {
                BT_LOGI("Searched service not found");
                service_search_pending = FALSE;
            }
            break;

        case BSA_BLE_CL_READ_EVT:
            BT_LOGI("BSA_BLE_CL_READ_EVT: conn_id:%d len:%d",
                    p_data->cli_read.conn_id, p_data->cli_read.len);
            client_num = app_ble_client_find_index_by_conn_id(blec, p_data->cli_read.conn_id);
            blec->ble_client[client_num].read_pending = FALSE;
            if(p_data->cli_read.status == BSA_SUCCESS)
            {
                //BT_LOGI("srvc_id:%s (uuid:0x%04x)",
                //        app_ble_display_service_name(p_data->cli_read.srvc_id.id.uuid.uu.uuid16),
                //        p_data->cli_read.srvc_id.id.uuid.uu.uuid16);

                //app_ble_client_handle_read_evt(&p_data->cli_read);
            }
            else
            {
                BT_LOGE("BSA_BLE_CL_READ_EVT: status:%d", p_data->cli_read.status);
            }
            if (blec->listener) {
                msg.cli_read.status = p_data->cli_read.status;
                msg.cli_read.len = p_data->cli_read.len;
                memcpy(msg.cli_read.value, p_data->cli_read.value, p_data->cli_read.len);
                bdcpy(msg.cli_read.bda, blec->ble_client[client_num].server_addr);

                 switch(p_data->cli_read.srvc_id.id.uuid.len) {
                    case LEN_UUID_16:
                        uuid16_to_string(msg.cli_read.service, p_data->cli_read.srvc_id.id.uuid.uu.uuid16);
                        break;
                    case LEN_UUID_32:
                        uuid32_to_string(msg.cli_read.service, p_data->cli_read.srvc_id.id.uuid.uu.uuid32);
                        break;
                    case LEN_UUID_128:
                        uuid128_to_string(msg.cli_read.service, p_data->cli_read.srvc_id.id.uuid.uu.uuid128);
                        break;
                }
                switch(p_data->cli_read.char_id.uuid.len) {
                    case LEN_UUID_16:
                        uuid16_to_string(msg.cli_read.char_id, p_data->cli_read.char_id.uuid.uu.uuid16);
                        break;
                    case LEN_UUID_32:
                        uuid32_to_string(msg.cli_read.char_id, p_data->cli_read.char_id.uuid.uu.uuid32);
                        break;
                    case LEN_UUID_128:
                        uuid128_to_string(msg.cli_read.char_id, p_data->cli_read.char_id.uuid.uu.uuid128);
                        break;
                }
                blec->listener(blec->listener_handle, BT_BLE_CL_READ_EVT, &msg);
            }
            break;

        case BSA_BLE_CL_CLOSE_EVT:
            BT_LOGI("BSA_BLE_CL_CLOSE_EVT, BDA:%02X:%02X:%02X:%02X:%02X:%02X  ",
                              p_data->cli_close.remote_bda[0], p_data->cli_close.remote_bda[1],
                              p_data->cli_close.remote_bda[2], p_data->cli_close.remote_bda[3],
                              p_data->cli_close.remote_bda[4], p_data->cli_close.remote_bda[5]);
            client_num = app_ble_client_find_index_by_interface(blec, p_data->cli_close.client_if);
            if(client_num < 0)
            {
                BT_LOGE("No client!!");
                break;
            }
            blec->ble_client[client_num].conn_id = BSA_BLE_INVALID_CONN;
            if (blec->listener) {
                BTDevice dev = {0};
                msg.cli_close.status = p_data->cli_close.status;
                bdcpy(msg.cli_close.bd_addr, p_data->cli_close.remote_bda);
                bdcpy(dev.bd_addr, p_data->cli_close.remote_bda);
                if (rokidbt_find_bonded_device(blec->caller, &dev) == 1) {
                    snprintf((char *)msg.cli_close.name, sizeof(msg.cli_close.name), "%s", dev.name);
                }
                blec->listener(blec->listener_handle, BT_BLE_CL_CLOSE_EVT, &msg);
            }
            break;


        case BSA_BLE_CL_WRITE_EVT:
            BT_LOGI("BSA_BLE_CL_WRITE_EVT status:%d", p_data->cli_write.status);
            client_num = app_ble_client_find_index_by_conn_id(blec, p_data->cli_write.conn_id);
            if (client_num >= 0)
            {
                blec->ble_client[client_num].write_pending = FALSE;
            }
            break;

        case BSA_BLE_CL_CONGEST_EVT:
            BT_LOGI("BSA_BLE_CL_CONGEST_EVT  :conn_id:0x%x, congested:%d",
                        p_data->cli_congest.conn_id, p_data->cli_congest.congested);
            client_num = app_ble_client_find_index_by_conn_id(blec, p_data->cli_congest.conn_id);
            if (client_num >= 0)
            {
                blec->ble_client[client_num].congested = p_data->cli_congest.congested;
            }
            break;

        case BSA_BLE_CL_NOTIF_EVT:
            BT_LOGD("BSA_BLE_CL_NOTIF_EVT BDA :%02X:%02X:%02X:%02X:%02X:%02X",
                        p_data->cli_notif.bda[0], p_data->cli_notif.bda[1],
                        p_data->cli_notif.bda[2], p_data->cli_notif.bda[3],
                        p_data->cli_notif.bda[4], p_data->cli_notif.bda[5]);
            BT_LOGD("conn_id:0x%x, svrc_id:0x%04x, char_id:0x%04x, descr_type:0x%04x, len:%d, is_notify:%d",
                        p_data->cli_notif.conn_id,
                        p_data->cli_notif.char_id.srvc_id.id.uuid.uu.uuid16,
                        p_data->cli_notif.char_id.char_id.uuid.uu.uuid16,
                        p_data->cli_notif.descr_type.uuid.uu.uuid16, p_data->cli_notif.len,
                        p_data->cli_notif.is_notify);
            //APP_DUMP("data", p_data->cli_notif.value, p_data->cli_notif.len);
            if(p_data->cli_notif.is_notify != TRUE)
            {
                /* this is indication, and need to send confirm */
                BT_LOGI("Receive Indication! send Indication Confirmation!");
                status = BSA_BleClIndConfInit(&ind_conf);
                if (status < 0)
                {
                    BT_LOGE("BSA_BleClIndConfInit failed: %d", status);
                    break;
                }
                ind_conf.conn_id = p_data->cli_notif.conn_id;
                memcpy(&(ind_conf.char_id), &(p_data->cli_notif.char_id), sizeof(tBTA_GATTC_CHAR_ID));
                status = BSA_BleClIndConf(&ind_conf);
                if (status < 0)
                {
                    BT_LOGE("BSA_BleClIndConf failed: %d", status);
                    break;
                }
                //app_ble_client_handle_indication(&p_data->cli_notif);
            }
            else  /* notification */
            {
                //app_ble_client_handle_notification(&p_data->cli_notif);
            }
            if (blec->listener) {
                bdcpy(msg.cli_notif.bda, p_data->cli_notif.bda);
                msg.cli_notif.len= p_data->cli_notif.len;
                memcpy(msg.cli_notif.value, p_data->cli_notif.value, p_data->cli_notif.len);
                switch(p_data->cli_notif.char_id.srvc_id.id.uuid.len) {
                    case LEN_UUID_16:
                        uuid16_to_string(msg.cli_notif.service, p_data->cli_notif.char_id.srvc_id.id.uuid.uu.uuid16);
                        break;
                    case LEN_UUID_32:
                        uuid32_to_string(msg.cli_notif.service, p_data->cli_notif.char_id.srvc_id.id.uuid.uu.uuid32);
                        break;
                    case LEN_UUID_128:
                        uuid128_to_string(msg.cli_notif.service, p_data->cli_notif.char_id.srvc_id.id.uuid.uu.uuid128);
                        break;
                }
                switch(p_data->cli_notif.char_id.char_id.uuid.len) {
                    case LEN_UUID_16:
                        uuid16_to_string(msg.cli_notif.char_id, p_data->cli_notif.char_id.char_id.uuid.uu.uuid16);
                        break;
                    case LEN_UUID_32:
                        uuid32_to_string(msg.cli_notif.char_id, p_data->cli_notif.char_id.char_id.uuid.uu.uuid32);
                        break;
                    case LEN_UUID_128:
                        uuid128_to_string(msg.cli_notif.char_id, p_data->cli_notif.char_id.char_id.uuid.uu.uuid128);
                        break;
                }
                blec->listener(blec->listener_handle, BT_BLE_CL_NOTIF_EVT, &msg);
            }
            break;

        case BSA_BLE_CL_CACHE_SAVE_EVT:
            BT_LOGI("BSA_BLE_CL_CACHE_SAVE_EVT");
            BT_LOGI("evt:0x%x, num_attr:%d, conn_id:0x%x",
                      p_data->cli_cache_save.evt ,
                      p_data->cli_cache_save.num_attr ,p_data->cli_cache_save.conn_id);
            BT_LOGD("BDA:%02X:%02X:%02X:%02X:%02X:%02X  ",
                      p_data->cli_cache_save.bd_addr[0], p_data->cli_cache_save.bd_addr[1],
                      p_data->cli_cache_save.bd_addr[2], p_data->cli_cache_save.bd_addr[3],
                      p_data->cli_cache_save.bd_addr[4], p_data->cli_cache_save.bd_addr[5]);
            for(index=0; index < p_data->cli_cache_save.num_attr;index++)
            {
                char uuid[MAX_LEN_UUID_STR];
                switch(p_data->cli_cache_save.attr[index].uuid.len) {
                    case LEN_UUID_16:
                        snprintf(uuid, sizeof(uuid), "%04X", p_data->cli_cache_save.attr[index].uuid.uu.uuid16);
                        break;
                    case LEN_UUID_32:
                        snprintf(uuid, sizeof(uuid), "%08X", p_data->cli_cache_save.attr[index].uuid.uu.uuid32);
                        break;
                    case LEN_UUID_128:
                        uuid128_to_string(uuid, p_data->cli_cache_save.attr[index].uuid.uu.uuid128);
                        break;
                    default:
                        snprintf(uuid, sizeof(uuid), "unknown");
                        break;
                }
                    BT_LOGI("\t Attr[0x%04x] handle[%d] uuid[%s] inst[%d] type[%d] prop[0x%1x] is_primary[%d]",
                                  index, p_data->cli_cache_save.attr[index].s_handle,
                                  uuid,
                                  p_data->cli_cache_save.attr[index].id,
                                  p_data->cli_cache_save.attr[index].attr_type,
                                  p_data->cli_cache_save.attr[index].prop,
                                  p_data->cli_cache_save.attr[index].is_primary);
            }
            /* save BLE DB */
            /* step 1. find element by bsa */
            /* step 2. add attribute */
            p_blecl_db_elmt = app_ble_client_db_find_by_bda(p_data->cli_cache_save.bd_addr);
            if (p_blecl_db_elmt == NULL)
            {
                BT_LOGI("No element!!");
                BT_LOGI("New device. Update BLE DB!!");
                /* update BLE DB */
                p_blecl_db_elmt = app_ble_client_db_alloc_element();
                if(p_blecl_db_elmt != NULL)
                {
                    bdcpy(p_blecl_db_elmt->bd_addr, p_data->cli_cache_save.bd_addr);
                    app_ble_client_db_add_element(p_blecl_db_elmt);
                    BT_LOGD("save BDA:%02X:%02X:%02X:%02X:%02X:%02X",
                          p_blecl_db_elmt->bd_addr[0], p_blecl_db_elmt->bd_addr[1],
                          p_blecl_db_elmt->bd_addr[2], p_blecl_db_elmt->bd_addr[3],
                          p_blecl_db_elmt->bd_addr[4], p_blecl_db_elmt->bd_addr[5]);
                    app_ble_client_db_save();
                }
            }
            if(p_blecl_db_elmt != NULL)
            {
                for(index=0; index < p_data->cli_cache_save.num_attr;index++)
                {
                    if(!app_ble_client_db_find_by_handle(p_blecl_db_elmt, p_data->cli_cache_save.attr[index].s_handle))
                    {
                        p_blecl_db_attr = app_ble_client_db_alloc_attr(p_blecl_db_elmt);
                        p_blecl_db_attr->attr_type = p_data->cli_cache_save.attr[index].attr_type;
                        p_blecl_db_attr->attr_UUID = p_data->cli_cache_save.attr[index].uuid;
                        p_blecl_db_attr->handle = p_data->cli_cache_save.attr[index].s_handle;
                        p_blecl_db_attr->id = p_data->cli_cache_save.attr[index].id;
                        p_blecl_db_attr->prop = p_data->cli_cache_save.attr[index].prop;
                        p_blecl_db_attr->is_primary = p_data->cli_cache_save.attr[index].is_primary;
                    }
                }
                /* step 3. save attribute */
                app_ble_client_db_save();
            }
            break;

        case BSA_BLE_CL_CACHE_LOAD_EVT:
            BT_LOGI("BSA_BLE_CL_CACHE_LOAD_EVT");
            BT_LOGD("BDA:%02X:%02X:%02X:%02X:%02X:%02X  id:0x%x",
                      p_data->cli_cache_load.bd_addr[0], p_data->cli_cache_load.bd_addr[1],
                      p_data->cli_cache_load.bd_addr[2], p_data->cli_cache_load.bd_addr[3],
                      p_data->cli_cache_load.bd_addr[4], p_data->cli_cache_load.bd_addr[5],
                      p_data->cli_cache_load.conn_id);
            /* search BLE DB */
            p_blecl_db_elmt = app_ble_client_db_find_by_bda(p_data->cli_cache_load.bd_addr);
            if(p_blecl_db_elmt != NULL)
            {
                BT_LOGD("YES! We have information of BDA:%02X:%02X:%02X:%02X:%02X:%02X  ",
                      p_data->cli_cache_load.bd_addr[0], p_data->cli_cache_load.bd_addr[1],
                      p_data->cli_cache_load.bd_addr[2], p_data->cli_cache_load.bd_addr[3],
                      p_data->cli_cache_load.bd_addr[4], p_data->cli_cache_load.bd_addr[5]);
                app_ble_client_load_attr(p_blecl_db_elmt, p_data->cli_cache_load.bd_addr, p_data->cli_cache_load.conn_id);
            }
            else
            {
                BT_LOGD("No information of BDA:%02X:%02X:%02X:%02X:%02X:%02X  ",
                      p_data->cli_cache_load.bd_addr[0], p_data->cli_cache_load.bd_addr[1],
                      p_data->cli_cache_load.bd_addr[2], p_data->cli_cache_load.bd_addr[3],
                      p_data->cli_cache_load.bd_addr[4], p_data->cli_cache_load.bd_addr[5]);
                app_ble_client_load_attr(p_blecl_db_elmt, p_data->cli_cache_load.bd_addr, p_data->cli_cache_load.conn_id);
            }
            break;

        case BSA_BLE_CL_CFG_MTU_EVT:
            BT_LOGI("BSA_BLE_CL_CFG_MTU_EVT");
            BT_LOGD("status:0x%x, conn_id:%d, mtu:%d", p_data->cli_cfg_mtu.status,
                p_data->cli_cfg_mtu.conn_id, p_data->cli_cfg_mtu.mtu);
            break;

        default:
            break;
    }
}

int app_ble_client_register(BleClient *blec, UINT16 uuid)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_REGISTER ble_appreg_param;
    int index;

    index = app_ble_client_find_free_space(blec);
    if( index < 0)
    {
        BT_LOGE("app_ble_client_register maximum number of BLE client already registered");
        return -1;
    }
    status = BSA_BleClAppRegisterInit(&ble_appreg_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BLE client registration failed with status %d", status);
        return -1;
    }

    ble_appreg_param.uuid.len = 2;
    ble_appreg_param.uuid.uu.uuid16 = uuid;
    ble_appreg_param.p_cback = app_ble_client_profile_cback;

    //BTM_BLE_SEC_NONE: No authentication and no encryption
    //BTM_BLE_SEC_ENCRYPT: encrypt the link with current key
    //BTM_BLE_SEC_ENCRYPT_NO_MITM: unauthenticated encryption
    //BTM_BLE_SEC_ENCRYPT_MITM: authenticated encryption
    ble_appreg_param.sec_act = BTM_BLE_SEC_NONE;
    
    status = BSA_BleClAppRegister(&ble_appreg_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_BleClAppRegister failed status = %d", status);
        return -1;
    }
    blec->ble_client[index].enabled = TRUE;
    blec->ble_client[index].client_if = ble_appreg_param.client_if;

    return 0;
}

int app_ble_client_deregister_all(BleClient *blec)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_DEREGISTER ble_appdereg_param;
    int index;

    for(index = 0;index < BSA_BLE_CLIENT_MAX;index++)
    {
        if(blec->ble_client[index].enabled)
        {
            if(blec->ble_client[index].conn_id != BSA_BLE_INVALID_CONN)
            {
                app_ble_client_close(blec, blec->ble_client[index].server_addr);
            }
            BT_LOGI("deregister app:%d",index);
            status = BSA_BleClAppDeregisterInit(&ble_appdereg_param);
            if (status != BSA_SUCCESS)
            {
                BT_LOGE("BSA_BleClAppDeregisterInit failed status = %d", status);
                return -1;
            }
            ble_appdereg_param.client_if = blec->ble_client[index].client_if;
            status = BSA_BleClAppDeregister(&ble_appdereg_param);
            if (status != BSA_SUCCESS)
            {
                BT_LOGE("BSA_BleAppDeregister failed status = %d", status);
                return -1;
            }
        }
    }

    return 0;
}

int rokid_ble_client_enable(BleClient *blec)
{
    int ret = -1;
    int index;

    BT_CHECK_HANDLE(blec);
    if (blec->enabled) return 0;
    ret = ble_enable(BLE_CLIENT_STATUS);
    if (ret) {
        BT_LOGE("failed to ble enable");
        return ret;
    }
     for (index = 0; index < BSA_BLE_CLIENT_MAX; index++)
    {
        blec->ble_client[index].client_if = BSA_BLE_INVALID_IF;
        blec->ble_client[index].conn_id = BSA_BLE_INVALID_CONN;
    }
    ret = app_ble_client_register(blec, APP_BLE_MAIN_DEFAULT_APPL_UUID);
    if (ret) {
        BT_LOGE("failed to app_ble_client_register");
        return ret;
    }
    app_dm_set_ble_bg_conn_type(1);//atuo
    blec->enabled = true;
    return ret;
}

int rokid_ble_client_disable(BleClient *blec)
{
    BT_CHECK_HANDLE(blec);
    if (blec->enabled) {
        app_ble_client_deregister_all(blec);
        ble_disable(BLE_CLIENT_STATUS);
        blec->enabled = false;
    }
    return 0;
}

BleClient *blec_create(void *caller)
{
    BleClient *blec = calloc(1, sizeof(*blec));

    blec->caller = caller;
    _ble_client = blec;
    return blec;
}

int blec_destroy(BleClient *blec)
{
    if (blec) {
        //rokid_ble_client_disable(blec);
        free(blec);
    }
    _ble_client = NULL;
    return 0;
}

int app_ble_client_open(BleClient *blec, int client_num, BD_ADDR bd_addr, bool is_direct)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_OPEN ble_open_param;

    BT_CHECK_HANDLE(blec);

    if (client_num < 0 ||client_num >= BSA_BLE_CLIENT_MAX  || (blec->ble_client[client_num].enabled == FALSE))
    {
        BT_LOGE("Wrong client number! = %d", client_num);
        return -1;
    }
    if (blec->ble_client[client_num].conn_id != BSA_BLE_INVALID_CONN)
    {
        BT_LOGE("Connection already exist, conn_id = %d", blec->ble_client[client_num].conn_id );
        return -1;
    }

    status = BSA_BleClConnectInit(&ble_open_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_BleClConnectInit failed status = %d", status);
        return -1;
    }

    ble_open_param.client_if = blec->ble_client[client_num].client_if;
    ble_open_param.is_direct = is_direct;

    //bdcpy(blec->ble_client[client_num].server_addr, bd_addr);
    bdcpy(ble_open_param.bd_addr, bd_addr);

    status = BSA_BleClConnect(&ble_open_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_BleClConnect failed status = %d", status);
        return -1;
    }

    return 0;
}

int app_ble_client_service_search_execute(BleClient *blec, int client_num, char *service)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_SEARCH ble_search_param;

    status = BSA_BleClSearchInit(&ble_search_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_BleClServiceSearchReqInit failed status = %d", status);
    }

    if((client_num < 0) || (client_num >= BSA_BLE_CLIENT_MAX))
    {
        BT_LOGE("BSA_BleClServiceSearchReq failed Invalid client_num:%d", client_num);
        return -1;
    }
    ble_search_param.conn_id= blec->ble_client[client_num].conn_id;
    if (service) {
        ble_search_param.uuid.len = LEN_UUID_128;
        if (string_to_uuid128(ble_search_param.uuid.uu.uuid128, service)) {
            return -1;
        }
    }
    status = BSA_BleClSearch(&ble_search_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_BleClServiceSearchReq failed with status:%d", status);
        return -1;
    }
    service_search_pending = TRUE;
    return 0;
}

int app_ble_client_read(BleClient *blec, int client_num, 
                        char *service, char *char_id, int ser_inst_id, int char_inst_id, int is_primary, char *descr_id)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_READ ble_read_param;
    int s_len = 0 , c_len = 0, d_len = 0;

    if((client_num < 0) ||
       (client_num >= BSA_BLE_CLIENT_MAX) ||
       (blec->ble_client[client_num].enabled == FALSE))
    {
        BT_LOGE("app_ble_client_read client %d was not enabled yet", client_num);
        return -1;
    }
    if (!service ||!char_id ) {
        BT_LOGE("failed service or char_id");
        return -1;
    }
    s_len = strlen(service);
    c_len = strlen(char_id);
    status = BSA_BleClReadInit(&ble_read_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("app_ble_client_read failed status = %d", status);
        return -1;
    }
    if(descr_id) {
        d_len = strlen(descr_id);
        switch(s_len) {
        case 6:
            break;
        case 10:
            break;
        case (MAX_LEN_UUID_STR - 1):
            break;
        default:
            return -1;
        }
        switch(c_len) {
        case 6:
            break;
        case 10:
            break;
        case (MAX_LEN_UUID_STR - 1):
            break;
        default:
            return -1;
        }
        switch(d_len) {
        case 6:
            break;
        case 10:
            break;
        case (MAX_LEN_UUID_STR - 1):
            break;
        default:
            return -1;
        }
        ble_read_param.descr_id.char_id.char_id.uuid.len = LEN_UUID_128;
        ble_read_param.descr_id.char_id.srvc_id.id.uuid.len = LEN_UUID_128;
        ble_read_param.descr_id.descr_id.uuid.len = LEN_UUID_128;
        if (string_to_uuid128(ble_read_param.descr_id.char_id.srvc_id.id.uuid.uu.uuid128, service)) {
            return -1;
        }
        if (string_to_uuid128(ble_read_param.descr_id.char_id.char_id.uuid.uu.uuid128, char_id)) {
            return -1;
        }
        if (string_to_uuid128(ble_read_param.descr_id.descr_id.uuid.uu.uuid128, descr_id)) {
            return -1;
        }

        ble_read_param.descr_id.char_id.srvc_id.is_primary = is_primary;
        ble_read_param.descr = TRUE;
    }
    else
    {
        switch(s_len) {
        case 6:
            ble_read_param.char_id.srvc_id.id.uuid.len = LEN_UUID_16;
            if (string_to_uuid16(&ble_read_param.char_id.srvc_id.id.uuid.uu.uuid16, service)) {
                    return -1;
            }
            break;
        case 10:
            ble_read_param.char_id.srvc_id.id.uuid.len = LEN_UUID_32;
            if (string_to_uuid32(&ble_read_param.char_id.srvc_id.id.uuid.uu.uuid32, service)) {
                    return -1;
            }
            break;
        case (MAX_LEN_UUID_STR - 1):
            ble_read_param.char_id.srvc_id.id.uuid.len = LEN_UUID_128;
            if (string_to_uuid128(ble_read_param.char_id.srvc_id.id.uuid.uu.uuid128, service)) {
                    return -1;
            }
            break;
        default:
            return -1;
        }
        switch(c_len) {
        case 6:
            ble_read_param.char_id.char_id.uuid.len = LEN_UUID_16;
             if (string_to_uuid16(&ble_read_param.char_id.char_id.uuid.uu.uuid16, char_id)) {
                    return -1;
            }
            break;
        case 10:
            ble_read_param.char_id.char_id.uuid.len = LEN_UUID_32;
             if (string_to_uuid32(&ble_read_param.char_id.char_id.uuid.uu.uuid32, char_id)) {
                    return -1;
            }
            break;
        case (MAX_LEN_UUID_STR - 1):
            ble_read_param.char_id.char_id.uuid.len = LEN_UUID_128;
            if (string_to_uuid128(ble_read_param.char_id.char_id.uuid.uu.uuid128, char_id)) {
                    return -1;
            }
            break;
        default:
            return -1;
        }

        ble_read_param.char_id.srvc_id.id.inst_id = ser_inst_id;
        ble_read_param.char_id.srvc_id.is_primary = is_primary;
        ble_read_param.char_id.char_id.inst_id = char_inst_id;
        ble_read_param.descr = FALSE;
    }
    ble_read_param.conn_id = blec->ble_client[client_num].conn_id;
    ble_read_param.auth_req = 0x00;
    status = BSA_BleClRead(&ble_read_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("app_ble_client_read failed status = %d", status);
        return -1;
    }
    return 0;
}


int app_ble_client_write(BleClient *blec, int client_num,
            char *service, char *char_id, int ser_inst_id, int char_inst_id, int is_primary, UINT8 *buff, int len,
            char *descr_id, UINT8 desc_value)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_WRITE ble_write_param;


    if((client_num < 0) ||
       (client_num >= BSA_BLE_CLIENT_MAX) ||
       (blec->ble_client[client_num].enabled == FALSE))
    {
        BT_LOGE("Wrong client number! = %d", client_num);
        return -1;
    }

    if(blec->ble_client[client_num].write_pending ||
        blec->ble_client[client_num].congested)
    {
        BT_LOGE("fail : write pending(%d), congested(%d)!",
            blec->ble_client[client_num].write_pending,
            blec->ble_client[client_num].congested);
        return -1;
    }
    if(len > BSA_BLE_CL_WRITE_MAX) {
        BT_LOGE("write len(%d) too long, max(%d)", len, BSA_BLE_CL_WRITE_MAX);
    }

    status = BSA_BleClWriteInit(&ble_write_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_BleClWriteInit failed status = %d", status);
        return -1;
    }

    if(descr_id) {
                ble_write_param.descr_id.char_id.char_id.uuid.len = LEN_UUID_128;
                ble_write_param.descr_id.char_id.srvc_id.id.uuid.len = LEN_UUID_128;
                ble_write_param.descr_id.descr_id.uuid.len = LEN_UUID_128;
                if (string_to_uuid128(ble_write_param.descr_id.char_id.srvc_id.id.uuid.uu.uuid128, service)) {
                    return -1;
                }
                if (string_to_uuid128(ble_write_param.descr_id.char_id.char_id.uuid.uu.uuid128, char_id)) {
                    return -1;
                }
                if (string_to_uuid128(ble_write_param.descr_id.descr_id.uuid.uu.uuid128, descr_id)) {
                    return -1;
                }

        ble_write_param.descr_id.char_id.srvc_id.is_primary = is_primary;
        ble_write_param.len = 1;
        ble_write_param.value[0] = desc_value;
        ble_write_param.descr = TRUE;
    }
    else
    {
                ble_write_param.char_id.srvc_id.id.uuid.len = LEN_UUID_128;
                ble_write_param.char_id.char_id.uuid.len = LEN_UUID_128;
                if (string_to_uuid128(ble_write_param.char_id.srvc_id.id.uuid.uu.uuid128, service)) {
                    return -1;
                }
                if (string_to_uuid128(ble_write_param.char_id.char_id.uuid.uu.uuid128, char_id)) {
                    return -1;
                }
        memcpy(ble_write_param.value, buff, len);
        ble_write_param.len = len;
        ble_write_param.descr = FALSE;
        ble_write_param.char_id.srvc_id.id.inst_id= ser_inst_id;
        ble_write_param.char_id.srvc_id.is_primary = is_primary;
        ble_write_param.char_id.char_id.inst_id = char_inst_id;
    }

    ble_write_param.conn_id = blec->ble_client[client_num].conn_id;
    ble_write_param.auth_req = BTA_GATT_AUTH_REQ_NONE;
    ble_write_param.write_type = BTA_GATTC_TYPE_WRITE;//BTA_GATTC_TYPE_WRITE_NO_RSP;

    status = BSA_BleClWrite(&ble_write_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_BleClWrite failed status = %d", status);
        return -1;
    }
    return 0;
}

int app_ble_client_register_notification(BleClient *blec, int client_num, char *service_id, char *character_id,
                    int ser_inst_id, int char_inst_id, int is_primary)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_NOTIFREG ble_notireg_param;

    if((client_num < 0) ||
       (client_num >= BSA_BLE_CLIENT_MAX) ||
       (blec->ble_client[client_num].enabled == FALSE))
    {
        BT_LOGE("Wrong client number! = %d", client_num);
        return -1;
    }
    status = BSA_BleClNotifRegisterInit(&ble_notireg_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_BleClNotifRegisterInit failed status = %d", status);
        return -1;
    }

                ble_notireg_param.notification_id.srvc_id.id.uuid.len = LEN_UUID_128;
                ble_notireg_param.notification_id.char_id.uuid.len  = LEN_UUID_128;
                if (string_to_uuid128(ble_notireg_param.notification_id.srvc_id.id.uuid.uu.uuid128, service_id)) {
                    return -1;
                }
                if (string_to_uuid128(ble_notireg_param.notification_id.char_id.uuid.uu.uuid128, character_id)) {
                    return -1;
                }

    ble_notireg_param.notification_id.srvc_id.id.inst_id = ser_inst_id;
    ble_notireg_param.notification_id.srvc_id.is_primary = is_primary;
    ble_notireg_param.notification_id.char_id.inst_id = char_inst_id;
    bdcpy(ble_notireg_param.bd_addr, blec->ble_client[client_num].server_addr);
    ble_notireg_param.client_if = blec->ble_client[client_num].client_if;

    status = BSA_BleClNotifRegister(&ble_notireg_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_BleClNotifRegister failed status = %d", status);
        return -1;
    }
    return 0;
}

int app_ble_client_close(BleClient *blec, BD_ADDR bd_addr)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_CLOSE ble_close_param;
    tAPP_BLE_CLIENT *client = NULL;
    
    client = app_ble_client_by_addr(blec, bd_addr);
    if (client == NULL) {
        BT_LOGE("no open yet!");
        return -1;
    }
    status = BSA_BleClCloseInit(&ble_close_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_BleClCLoseInit failed status = %d", status);
        return -1;
    }
    ble_close_param.conn_id = client->conn_id;
    status = BSA_BleClClose(&ble_close_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_BleClClose failed status = %d", status);
        return -1;
    }

    return 0;
}


int app_ble_client_unpair(BleClient *blec, BD_ADDR bd_addr)
{
    tAPP_BLE_CLIENT_DB_ELEMENT *p_blecl_db_elmt;
    tAPP_BLE_CLIENT *client = NULL;

    client = app_ble_client_by_addr(blec, bd_addr);
    if (client == NULL) {
        BT_LOGE("no enable yet!");
        return 0;
    }
    /*First close any active connection if exist*/
    if(client->conn_id != BSA_BLE_INVALID_CONN)
    {
        app_ble_client_close(blec, bd_addr);
    }
    //rokidbt_mgr_sec_unpair(blec->caller, bd_addr);

    /* search BLE DB */
    p_blecl_db_elmt = app_ble_client_db_find_by_bda(bd_addr);
    if(p_blecl_db_elmt != NULL)
    {
        BT_LOGI("Device present in DB, So clear it!!");
        app_ble_client_db_clear_by_bda(bd_addr);
        app_ble_client_db_save();
    }
    else
    {
        BT_LOGI("No device present");
    }
    return 0;
}


int app_ble_client_deregister_notification(BleClient *blec, int client_num, char *service_id, char *character_id,
                    int ser_inst_id, int char_inst_id, int is_primary)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_NOTIFDEREG ble_notidereg_param;

    if((client_num < 0) ||
       (client_num >= BSA_BLE_CLIENT_MAX) ||
       (blec->ble_client[client_num].enabled == FALSE))
    {
        BT_LOGE("Wrong client number! = %d", client_num);
        return -1;
    }
    status = BSA_BleClNotifDeregisterInit(&ble_notidereg_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_BleClNotifDeregisterInit failed status = %d", status);
        return -1;
    }

    bdcpy(ble_notidereg_param.bd_addr, blec->ble_client[client_num].server_addr);
    ble_notidereg_param.client_if = blec->ble_client[client_num].client_if;

    if(service_id) {
                ble_notidereg_param.notification_id.srvc_id.id.uuid.len = LEN_UUID_128;
                ble_notidereg_param.notification_id.char_id.uuid.len  = LEN_UUID_128;
                if (string_to_uuid128(ble_notidereg_param.notification_id.srvc_id.id.uuid.uu.uuid128, service_id)) {
                    return -1;
                }
                if (string_to_uuid128(ble_notidereg_param.notification_id.char_id.uuid.uu.uuid128, character_id)) {
                    return -1;
                }

        ble_notidereg_param.notification_id.srvc_id.id.inst_id = ser_inst_id;
        ble_notidereg_param.notification_id.srvc_id.is_primary = is_primary;
        ble_notidereg_param.notification_id.char_id.inst_id = char_inst_id;
        bdcpy(ble_notidereg_param.bd_addr, blec->ble_client[client_num].server_addr);
        ble_notidereg_param.client_if = blec->ble_client[client_num].client_if;
    }
    status = BSA_BleClNotifDeregister(&ble_notidereg_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_BleClNotifDeregister failed status = %d", status);
        return -1;
    }
    return 0;
}

int ble_client_set_listener(BleClient *blec, ble_callbacks_t listener, void *listener_handle)
{
    BT_CHECK_HANDLE(blec);
    blec->listener = listener;
    blec->listener_handle = listener_handle;
   return 0;
}

int ble_client_get_connect_devs(BleClient *blec, BTDevice *devs, int arr_len)
{
    int count = 0;
    int index;
    BT_CHECK_HANDLE(blec);

    for (index = 0;index < BSA_BLE_CLIENT_MAX;index++) {
        if (blec->ble_client[index].enabled) {
            if (blec->ble_client[index].conn_id != BSA_BLE_INVALID_CONN) {
                BTDevice *dev = devs + count;
                snprintf(dev->name, sizeof(dev->name), "%s", blec->ble_client[index].name);
                bdcpy(dev->bd_addr, blec->ble_client[index].server_addr);
                count++;
            }
            if (count >= arr_len) break;
        }
    }
    return count;
}

int app_ble_hogp_register_notification(BleClient *blec, int client)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_NOTIFREG ble_notireg_param;

    BT_CHECK_HANDLE(blec);
    status = BSA_BleClNotifRegisterInit(&ble_notireg_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_BleClNotifRegisterInit failed status = %d", status);
    }

    /* register all notification */
    bdcpy(ble_notireg_param.bd_addr, blec->ble_client[client].server_addr);
    ble_notireg_param.client_if = blec->ble_client[client].client_if;

    ble_notireg_param.notification_id.srvc_id.id.uuid.len = 2;
    ble_notireg_param.notification_id.srvc_id.id.uuid.uu.uuid16 = 0x1812;
    ble_notireg_param.notification_id.srvc_id.is_primary = 1;

    ble_notireg_param.notification_id.char_id.uuid.len = 2;
    ble_notireg_param.notification_id.char_id.uuid.uu.uuid16 = 0x2a4d;

    status = BSA_BleClNotifRegister(&ble_notireg_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_BleClNotifRegister failed status = %d", status);
        return -1;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_ble_hogp_enable_notification_monitor
 **
 ** Description     
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_hogp_enable_notification(BleClient *blec, int client)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_WRITE ble_write_param;
    UINT16 char_id, service;

    status = BSA_BleClWriteInit(&ble_write_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_BleClWriteInit failed status = %d", status);
    }

    service = 0x1812; 
    char_id = 0x2a4d;

    ble_write_param.descr_id.char_id.char_id.uuid.len=2;
    ble_write_param.descr_id.char_id.char_id.uuid.uu.uuid16 = char_id;
    ble_write_param.descr_id.char_id.srvc_id.id.uuid.len=2;
    ble_write_param.descr_id.char_id.srvc_id.id.uuid.uu.uuid16 = service;
    ble_write_param.descr_id.char_id.srvc_id.is_primary = 1;
    ble_write_param.descr_id.descr_id.uuid.uu.uuid16 = BSA_BLE_GATT_UUID_CHAR_CLIENT_CONFIG;
    ble_write_param.descr_id.descr_id.uuid.len = 2;
    ble_write_param.len = 2;
    ble_write_param.value[0] = BSA_BLE_GATT_UUID_CHAR_CLIENT_CONFIG_ENABLE_NOTI;
    ble_write_param.value[1] = 0;
    ble_write_param.descr = TRUE;
    ble_write_param.conn_id = 5;//blec->ble_client[client].conn_id;

    ble_write_param.auth_req = BTA_GATT_AUTH_REQ_NONE;
    ble_write_param.write_type = BTA_GATTC_TYPE_WRITE;

    status = BSA_BleClWrite(&ble_write_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_BleClWrite failed status = %d", status);
        return -1;
    }
    return 0;
}

