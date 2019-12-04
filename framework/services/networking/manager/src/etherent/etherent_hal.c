#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <common.h>
#include <hardware/ethnet_hal.h>
#include <hardware/hardware.h>

static int g_start_etherent_flag = 0; 
static struct eth_module_t *module = NULL;
static struct eth_device_t *etherent_device = NULL;

static inline int etherent_device_open(const hw_module_t *module, struct eth_device_t **device) 
{
  return module->methods->open(module, ETHNET_HARDWARE_MODULE_ID,(struct hw_device_t **)device);
}

int etherent_hal_init(void)
{
   if(hw_get_module(ETHNET_HARDWARE_MODULE_ID,(const struct hw_module_t **)&module))
   {
          NM_LOGE("hw_get_module run fail\n");
          return -1;
   }

   if(etherent_device_open(&module->common,&etherent_device))
   {
        NM_LOGE("modem_dev_open fail\n");
        return -1;
   }

   return 0;
}

void etherent_hal_exit(void)
{
    etherent_device->common.close(&etherent_device->common);
}

int etherent_hal_get_capacity(void)
{
    return  etherent_device->get_status();
}

int etherent_hal_start_eth(void)
{
   int iret=0;

   if(g_start_etherent_flag == 0)
   {
        g_start_etherent_flag = 1;
        iret = etherent_device->on();
        iret |= etherent_device->start_dhcpcd();
   }

   return iret;
}
 
void etherent_hal_stop_eth(void)
{
   if(g_start_etherent_flag  == 1)
   {
      etherent_device->stop_dhcpcd();
      etherent_device->off();
      g_start_etherent_flag = 0;
   }

   return ;
}
