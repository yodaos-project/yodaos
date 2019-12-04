#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <getopt.h>
#include <common.h>
#include <hardware/modem_tty.h>
#include <hardware/hardware.h>

static int g_start_modem_flag = 0;
static struct modem_module_t *module = NULL;
static struct modemtty_device_t *modem_device = NULL;
//static pthread_mutex_t modem_mutex  = PTHREAD_MUTEX_INITIALIZER;

static inline int modem_dev_open(const hw_module_t *module, struct modemtty_device_t **device) 
{
	return module->methods->open(module,MODEM_TTY_HW_ID,(struct hw_device_t **)device);
}


int modem_hal_init(void)
{
  if(hw_get_module( MODEM_TTY_HW_ID,(const struct hw_module_t **)&module))
  {
		NM_LOGE("hw_get_module run fail\n");
        return -1;
  }

  if(modem_dev_open(&module->common, &modem_device))
  { 
	  NM_LOGE("modem_dev_open fail\n");
	  return -1;
  }

   return 0;
}

void modem_hal_exit(void)
{
    modem_device->common.close(&modem_device->common);
}

int modem_hal_get_capacity(void)
{
   return  modem_device->get_status();
}

int modem_hal_start_modem(void)
{
   int iret=0;

   if(g_start_modem_flag == 0)
   {
      g_start_modem_flag = 1;
 //     pthread_mutex_lock(&modem_mutex);
      iret = modem_device->on();
//      pthread_mutex_unlock(&modem_mutex);
      iret |= modem_device->start_dhcpcd();
   }
   return iret;
}

void modem_hal_stop_modem(void)
{
  if(g_start_modem_flag == 1)
  {
     modem_device->stop_dhcpcd();
 //    pthread_mutex_lock(&modem_mutex);
     modem_device->off();
  //   pthread_mutex_unlock(&modem_mutex);
     (void)system("ifconfig eth0 down");
     g_start_modem_flag = 0;
  }
  return ;
}


int modem_hal_get_vendor(sMODEM_VENDOR_INFO *vendor)
{
   return modem_device->get_vendor(vendor);
}

int modem_hal_get_signal_type(eMODEM_SIGNAL_TYPE*type,eMODEM_SIGNAL_VENDOR *vendor)
{
    int iret;

//    pthread_mutex_lock(&modem_mutex);
    iret =  modem_device->get_signal_type(type,vendor);
 //   pthread_mutex_unlock(&modem_mutex);

    return iret;
}

int modem_hal_get_signal_strength(int *signal)
{
     int iret;
  //   pthread_mutex_lock(&modem_mutex);
     iret =  modem_device->get_signal_strength(signal);
  //  pthread_mutex_unlock(&modem_mutex);
    return iret;
}
