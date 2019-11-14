#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <common.h>
#include <hardware/wifi_hal.h>
#include <hardware/hardware.h>

static int g_start_ap_flag = 0;
static int g_start_station_flag = 0;
static struct wifi_module_t *module = NULL;
static struct wifi_device_t *wifi_device = NULL;

static inline int wifi_device_open(const hw_module_t *module, struct wifi_device_t **device) 
{
	return module->methods->open(module, WIFI_HARDWARE_MODULE_ID,(struct hw_device_t **)device);
}

int wifi_hal_init(void)
{
    if(hw_get_module(WIFI_HARDWARE_MODULE_ID,(const struct hw_module_t **)&module))
    {
         NM_LOGE("hw_get_module run fail\n");
         return -1;
     }

    if(wifi_device_open(&module->common,&wifi_device))
    {
       NM_LOGE("modem_dev_open fail\n");
       return -1;
    }
  return 0;
}

void wifi_hal_exit(void)
{
   wifi_device->common.close(&wifi_device->common);
}

int wifi_hal_get_capacity(void)
{
  return  wifi_device->get_status();
}

int wifi_hal_start_station(void) 
{
   int iret = 0;

   if(g_start_station_flag == 0)
   {
     g_start_station_flag  =1;
     iret = wifi_device->start_station();
     iret |= system("/etc/init.d/dhcpcd restart");
     iret |= wifi_device->on();
   }

   return iret;
}

void wifi_hal_stop_station(void) 
{
  if(g_start_station_flag)
  {
     system("/etc/init.d/dhcpcd stop");
     wifi_device->stop_station();
     sleep(2);
     wifi_device->off();
     g_start_station_flag = 0;
  }
}

int wifi_hal_start_ap(char *ssid,char *psk,char *ip)
{
  int iret = 0;

  if(g_start_ap_flag == 0)
  {
     g_start_ap_flag = 1;
    wifi_device->on();
     iret =  wifi_device->start_ap(ssid,psk,ip);
  }
  return iret;
}

void wifi_hal_stop_ap(void)
{
  if(g_start_ap_flag)
  { 
    g_start_ap_flag = 0;
    wifi_device->stop_ap();
    sleep(2);
    wifi_device->off();
  }
}

