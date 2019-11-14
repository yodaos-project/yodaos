#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <termio.h>
#include <getopt.h>
#include <inttypes.h>
#include <pthread.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/statfs.h>
#include <cutils/log.h>
#include <sys/times.h> 
#include <sys/select.h> 
#include <json-c/json.h>
#include <cutils/properties.h>
#include <libusb-1.0/libusb.h>
#include <hardware/hardware.h>
#include <rokidware/modem_log.h>
#include <rokidware/modem_tty.h>

#ifndef MODEM_TTY_DEV
#define MODEM_TTY_DEV "/dev/ttyACM1"
#endif

#ifndef MODEM_TTY_BAUD
#define MODEM_TTY_BAUD 115200
#endif

#define USB_VID_KEY               "VID"
#define USB_PID_KEY               "PID"
#define MODULE_NAME_KEY           "MODULE"
#define AT_FILENAME_KEY           "FILE"
#define MODEM_ON_KEY              "ON"
#define MODEM_OFF_KEY             "OFF"
#define MODEM_STATE_KEY           "STATE"
#define MODEM_VENDOR_KEY          "VENDOR"
#define MODEM_ETH_KEY             "ETH"
#define USB_DEV_KEY               "USB"
#define MODEM_SIGNAL_TYPE_KEY     "TYPE"
#define MODEM_SIGNAL_STRENGTH_KEY "STRENGTH"

#define  MODEM_INFO_DIR     "/etc/modem/"
#define  MODEM_MODULE_FILE  "modems.json"

#define MAX_REPEAT_TIMES (6)
#define AT_CMD_RESULT_OK "OK"

#define MAX_TTY_LEN 4096 
#define MAX_USB_LIST_LEN (32)
#ifdef MODEM_TTY_DEBUG

#define DUMP_SRING(h,n)     \
   printf("%s:%s\n",#n,h->n)
#endif

#define json_store_string(dst,head_obj,key)                              \
     do{                                                                 \
           json_object* obj_elem;                                        \
          if((json_object_object_get_ex(head_obj,key,&obj_elem))!= 0)    \
            strncpy(dst,json_object_get_string(obj_elem),MAX_LEN-1);     \
     }while(0)

#define json_store_uint16(dst,head_obj,key)                                       \
     do{                                                                          \
           json_object* obj_elem;                                                 \
          if((json_object_object_get_ex(head_obj,key,&obj_elem))!= 0)             \
            dst = (uint16_t)strtol(json_object_get_string(obj_elem),NULL,16);     \
     }while(0)
 
struct usb_st
{
    uint16_t pid;
    uint16_t vid;
};

struct usb_json_idx
{
    uint16_t vid;
    uint16_t pid;
    char name[MAX_LEN];
    char file[MAX_LEN];  
};

struct usb_json
{
    int total;
    struct usb_json_idx*usb;  
};

struct at_json_array
{
   int total;
   uint8_t (*p)[MAX_LEN];
};

struct at_json
{
  char name[MAX_LEN];
  char state[MAX_LEN];
  char strength[MAX_LEN];
  char type[MAX_LEN];
  char vendor[MAX_LEN];
  char eth[MAX_LEN]; 
  struct at_json_array on;
  struct at_json_array off; 
};

static int tty_fd = -1;
static struct usb_json usb_json;
static struct at_json  at_json;

static char tty_buf[MAX_TTY_LEN];
static pthread_mutex_t tty_mutex = PTHREAD_MUTEX_INITIALIZER;

int modem_log_level = LEVEL_OVER_LOGI;


static int modem_set_bug_level(void)
{
     char value[64] = {0};

     property_get("persist.rokid.debug.modem", value, "0");
     if (atoi(value) > 0)
         modem_log_level = atoi(value);

     return 0;
}

static ssize_t ls_usb_device(struct usb_st *st)
{
    ssize_t list_len=0,idx=0;
    struct libusb_device **list=NULL;
    struct libusb_device *dev=NULL;
    struct libusb_device_descriptor desc={0};
    struct libusb_device_handle *usb_p=NULL;
 
    list_len=libusb_get_device_list(NULL,&list);

    if(list_len==0)
    {
        MODEM_LOGE("can't find usb list");
        return 0;
    }
   
    if(list_len > MAX_USB_LIST_LEN)
    {
       MODEM_LOGE("list_len(%d) > MAX_LEN(%d)\n",list_len,MAX_USB_LIST_LEN);
       return 0;
    }

    for(idx=0;idx <list_len;idx++)
    {
        memset(&desc,sizeof(struct libusb_device_descriptor),0);
        if(libusb_get_device_descriptor(list[idx],&desc)!=0)
        {
            MODEM_LOGE("can't find usb list information");
            return 0;
        }
        st[idx].pid=desc.idProduct;
        st[idx].vid=desc.idVendor;
    }
    return list_len;
}

static int get_usb_pid_vid_list(struct usb_st *st)
{
    int i;
    ssize_t list_len=0;

    libusb_init(NULL);

    list_len=ls_usb_device(st);

    libusb_exit(NULL);

    return list_len;
}
 
 
#ifdef MODEM_TTY_DEBUG
static void dump(uint8_t *buf ,uint32_t n)
{
 int i;

 for(i=0;i<n;i++)
 {
   if(i%16==0)
    printf("%02x:",i);
  
   printf("%02x ",buf[i]);

  if((i+1) %16== 0 )
    printf("\n");
 }
}
#endif

static int modem_tty_init(int * tty_fd)
{
   int fd;
   speed_t speed;
   struct termios ttyAttr;

   MODEM_LOGI("open %s\n",MODEM_TTY_DEV);

   fd = open(MODEM_TTY_DEV,O_RDWR|O_NOCTTY,0);
   
   if(fd < 0)
   {
         MODEM_LOGE("Unable to open %s\n",MODEM_TTY_DEV);
         return -1;
   }

   memset(&ttyAttr, 0, sizeof(struct termios));

   tcgetattr(fd,&ttyAttr);
 
   switch(MODEM_TTY_BAUD)
   {
     case 1800:
      speed = B1800;break;
     case 2400:
      speed = B2400;break;
     case 4800:
      speed = B4800;break;
     case 9600:
      speed = B9600;break;
     case 38400:
      speed = B38400;break;
     case 57600:
      speed = B57600;break;
     case 115200:
      speed = B115200;break;
     default:
      speed = B9600;break;
   }

   cfsetospeed(&ttyAttr,speed);
   cfsetispeed(&ttyAttr,speed);

   ttyAttr.c_cflag |= CREAD | CLOCAL;
   ttyAttr.c_cflag &= ~PARENB;
   ttyAttr.c_cflag &= ~CSTOPB;
   ttyAttr.c_cflag &= ~CSIZE;
   ttyAttr.c_cflag |= CS8; 
   ttyAttr.c_cflag |= HUPCL; 
   ttyAttr.c_oflag &= ~OPOST;

   ttyAttr.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

   ttyAttr.c_cc[VMIN] = 1;

   if(tcsetattr(fd, TCSANOW, &ttyAttr) < 0)
   {
        MODEM_LOGE("Unable to set comm port");
        return -1;
   }

   *tty_fd = fd;

   return 0;
}

static int xfer_tty_cmd(int fd,uint8_t *send_buf,int send_len,uint8_t *recv_buf,int recv_len)
{
    int n;
    int iret=0;
	fd_set read_set;

	struct timeval timeout = 
    {
		.tv_sec  = 3,
		.tv_usec = 0
	};

    pthread_mutex_lock(&tty_mutex);

	iret = write(fd,send_buf,send_len);

	if(iret < 0)	
	{
		MODEM_LOGE("write device failed\n");
        pthread_mutex_unlock (&tty_mutex);
        return -1;
	}

    usleep(10000);

    FD_ZERO(&read_set);
    FD_SET(fd,&read_set);

	if (select(fd+1,&read_set,NULL,NULL,&timeout) <= 0)
    {
       MODEM_LOGE("respond timeout\n");
       pthread_mutex_unlock (&tty_mutex);
       return -1;
    }
          
    if(FD_ISSET(fd,&read_set))
	{
		if((n=read(fd,recv_buf,recv_len-1)) <= 0 )
		{
            iret = -1;
		}

        iret  = 0;
	}

    pthread_mutex_unlock (&tty_mutex);

#ifdef MODEM_TTY_DEBUG
    printf("send:%s,recv:%s\n",send_buf,recv_buf);
    printf("\n--------------------------------------------\n");
    dump(send_buf ,strlen(send_buf)+1);
    printf("\n--------------------------------------------\n");
    dump(recv_buf ,strlen(recv_buf)+1);
    printf("\n--------------------------------------------\n");
#endif

    return iret;
}


static int modem_send_cmd_for_status(int fd,uint8_t *send_buf)
{
  int cnts = 0;

  while(cnts < MAX_REPEAT_TIMES)
  {
    memset(tty_buf,0,MAX_TTY_LEN);

    if(xfer_tty_cmd(fd,send_buf,strlen(send_buf),tty_buf,MAX_TTY_LEN) ==0)
    {
         if(strstr(tty_buf,AT_CMD_RESULT_OK))
          return 0;
    }

    cnts ++;
  }
  return -1;
}

static int modem_send_cmd_for_result(int fd,uint8_t *send_buf,uint8_t* recv_buf,uint32_t recv_len)
{

  int cnts = 0;

  while( cnts < MAX_REPEAT_TIMES)
  {
    memset(recv_buf,0,recv_len);

    if(xfer_tty_cmd(fd,send_buf,strlen(send_buf),recv_buf,recv_len) ==0)
    {
         if(strstr(recv_buf,AT_CMD_RESULT_OK))
            return 0;
    }

    cnts ++;
  }

  return -1;
}

static int json_parse_module_file(struct usb_json* p_usb_json)
{
  int i;
  int found;
  char name[128];
  int array_len;
  struct usb_json_idx *p_json_idx;
  json_object*obj_head,*usb_array,*obj_usb,*obj_elem;

  memset(name,128,0);
  snprintf(name,128,"%s%s",MODEM_INFO_DIR,MODEM_MODULE_FILE);

  obj_head=json_object_from_file(name);

  if(obj_head ==NULL)
  {
    MODEM_LOGE("json_object_from_file %s fail\n",name);
    return -1;
  }

  found=json_object_object_get_ex(obj_head,USB_DEV_KEY,&usb_array);

  if(!found)
  {
    MODEM_LOGE("usb object get fail\n");
    return -1;
  }

  array_len = json_object_array_length(usb_array);

  if(array_len==0)
  {
    MODEM_LOGE("usb array len=0\n");
    return -1;
  }

  p_json_idx = (struct usb_json_idx*)malloc(sizeof(struct usb_json_idx)*array_len);

  if(p_json_idx ==NULL)
  {
    MODEM_LOGE("malloc usb_json_idx faild\n");
    return -1;
  }

  memset(p_json_idx,0,sizeof(struct usb_json_idx)*array_len);

  p_usb_json->total = array_len;
  p_usb_json->usb=p_json_idx;

  for(i=0;i<array_len;i++)
  {
     obj_usb =  json_object_array_get_idx(usb_array,i);

    json_store_uint16(p_usb_json->usb[i].vid,obj_usb,USB_VID_KEY);
    json_store_uint16(p_usb_json->usb[i].pid,obj_usb,USB_PID_KEY);
  
    json_store_string(p_usb_json->usb[i].name,obj_usb,MODULE_NAME_KEY);
    json_store_string(p_usb_json->usb[i].file,obj_usb,AT_FILENAME_KEY);
  }

  json_object_put(obj_head);

  return 0;
}

static int find_json_idx_by_usb(struct usb_json_idx * usb)
{
  int iret = -1;
  int i,j,json_total,usb_total;
  struct usb_st usb_st[MAX_USB_LIST_LEN];
  struct usb_json *p_usb_json= &usb_json;

  usb_total = get_usb_pid_vid_list(usb_st);

  if(json_parse_module_file(p_usb_json))
  {
    MODEM_LOGE("json_parse_module_file fail\n");
    return -1;
  }

  json_total = p_usb_json->total;

  for(i=0;i<json_total;i++)
    for(j=0;j<usb_total;j++)
   {
      if(p_usb_json->usb[i].vid ==usb_st[j].vid && 
      p_usb_json->usb[i].pid == usb_st[j].pid)
      {
        iret = 0;
        memcpy(usb,&p_usb_json->usb[i],sizeof(struct usb_json_idx));
        break;
      }
   }
   
   free(p_usb_json->usb);

   return iret;
}


static int json_store_string_array(struct at_json_array * p_json_array,char *key,json_object*obj_head)
{
  int i;
  int array_len;
  uint8_t(*p_array)[MAX_LEN];
  json_object*array,*obj;

  if(json_object_object_get_ex(obj_head,key,&array) == 0)
  {
    MODEM_LOGE("usb object get fail\n");
    return -1;
  }

  array_len = json_object_array_length(array);

  if(array_len)
  {
    p_array = (uint8_t (*)[MAX_LEN])malloc(array_len*MAX_LEN);

    if(p_array ==NULL)
    {
      MODEM_LOGE("malloc array faild\n");
      return -1;
    }

    memset(p_array,0,sizeof(MAX_LEN*array_len));

    p_json_array->total = array_len;
    p_json_array->p = p_array;
  }
  else
  {
    p_json_array->total = 0;
    p_json_array->p = NULL;
  }

  for(i=0;i<array_len;i++)
  {
     obj = json_object_array_get_idx(array,i);
     strncpy(p_array[i],json_object_get_string(obj),MAX_LEN-1);
  }

  return 0;
}

static int json_parse_at_file(struct at_json* p_at_json)
{
  int i;
  int found;
  char name[128];

  struct usb_json_idx usb_idx;
  json_object*obj_head,*array,*obj,*obj_elem;

  memset(name,128,0);
  memset(&usb_idx,0,sizeof(struct usb_json_idx));

  if(find_json_idx_by_usb(&usb_idx))
  {
    MODEM_LOGE("fail to find at cmd file mirror to vid pid\n");
    return -1;
  }

  snprintf(name,128,"%s%s",MODEM_INFO_DIR,usb_idx.file);

  obj_head=json_object_from_file(name);

  if(obj_head ==NULL)
  {
    MODEM_LOGE("json_object_from_file %s fail\n",name);
    return -1;
  }

  if(json_object_object_get_ex(obj_head,MODULE_NAME_KEY,&obj)==0)
  {
    MODEM_LOGE("object get %s fail\n",MODULE_NAME_KEY);
    return -1;
  }

  if(strcmp(usb_idx.name,json_object_get_string(obj)) != 0)
  {
    MODEM_LOGE("%s:module name:%s,is not same with %s:module name:%s\n",
          MODEM_MODULE_FILE,usb_idx.name,usb_idx.file,json_object_get_string(obj));

    return -1;
  }


  if(json_store_string_array(&p_at_json->on,MODEM_ON_KEY,obj_head))
  {
     MODEM_LOGE("json_store_string_array %s fail\n",MODEM_ON_KEY);
     return -1;
  }

  if(json_store_string_array(&p_at_json->off,MODEM_OFF_KEY,obj_head))
  {
     MODEM_LOGE("json_store_string_array %s fail\n",MODEM_OFF_KEY);
     return -1;
  }

  json_store_string(p_at_json->name,obj_head,MODULE_NAME_KEY);
  json_store_string(p_at_json->state,obj_head,MODEM_STATE_KEY);
  json_store_string(p_at_json->strength,obj_head,MODEM_SIGNAL_STRENGTH_KEY);
  json_store_string(p_at_json->type,obj_head,MODEM_SIGNAL_TYPE_KEY);
  json_store_string(p_at_json->vendor,obj_head,MODEM_VENDOR_KEY);
  json_store_string(p_at_json->eth,obj_head,MODEM_ETH_KEY);
  json_object_put(obj_head);

  return 0;
}

static int modem_dev_close(struct modemtty_device_t *device)
{
    struct at_json *p_at_json = &at_json;

    MODEM_LOGI("%s\n", __func__);

    if(p_at_json->on.p)
    {
      free(p_at_json->on.p);
      p_at_json->on.p = NULL;
    }

    if(p_at_json->off.p)
    {
      free(p_at_json->off.p);
      p_at_json->off.p = NULL;
    }

    close(tty_fd);
    free(device);
    return 0;
}

static int reset_modem(void)
{
  return 0;
}

static int modem_on(void)
{
  int i;
  struct at_json_array *p_array  = &at_json.on; 

  reset_modem();
  for(i=0;i<p_array->total;i++)
  {

    if(p_array->p[i]== NULL)
     continue;

    if(modem_send_cmd_for_status(tty_fd,p_array->p[i]) != 0)
    {
       MODEM_LOGE("AT CMD:%s return fail!\n",p_array->p[i]);
       return -1;
    }

  }

  return 0;
}

static int modem_off(void)
{
  int i;
  struct at_json_array *p_array  = &at_json.off; 

  reset_modem();

  for(i=0;i<p_array->total;i++)
  {
    if(p_array->p[i]== NULL)
     continue;

    if(modem_send_cmd_for_status(tty_fd,p_array->p[i]) != 0)
    {
       MODEM_LOGE("AT CMD:%s return fail!\n",p_array->p[i]);
       return -1;
    }

  }

  return 0;
}

/** 
  O,OK
  -1,Error
**/
static int modem_get_state(void)
{
  struct at_json *p_json = &at_json;

  return modem_send_cmd_for_status(tty_fd,p_json->state);
}

#define CSQ_HEAD "+CSQ: "

static int get_strength_value(char *string)
{
  char *head = CSQ_HEAD;
  int val=0;

  char *p = strstr(string,head);
 
  if(p)
     val=atoi(&p[strlen(head)]);

  return val;

}
 

static int modem_get_signal_strength(int*signal)
{
  int iret = -1;
  struct at_json *p_json = &at_json;

  memset(tty_buf,0,MAX_TTY_LEN);
  if(modem_send_cmd_for_result(tty_fd,p_json->strength,tty_buf,MAX_TTY_LEN)==0)
  {
    *signal = get_strength_value(tty_buf);  
    iret  = 0;
  }
 
  return iret;
}

#define SYS_INFO_HEAD "$MYSYSINFO: "

static int get_type_value(char *sysinf,eMODEM_SIGNAL_TYPE *type,eMODEM_SIGNAL_VENDOR *vendor)
{
     char *head = SYS_INFO_HEAD;
     int t = eSIGNAL_SERVICE_NONE;
     int v = eSIGNAL_REG_FAIL;
     char *p = strstr(sysinf,head);
 
     if(p)
     {
        t=atoi(&p[strlen(head)]);
        v=atoi(&p[strlen(head)+2]);
     }
     if(t != eSIGNAL_SERVICE_NONE &&
        t != eSIGNAL_SERVICE_2G   &&
        t != eSIGNAL_SERVICE_3G   &&
        t != eSIGNAL_SERVICE_4G )
        t = eSIGNAL_SERVICE_NONE;
 
    if(v != eSIGNAL_REG_FAIL      &&
       v != eSIGNAL_IN_CMCC_MODE  &&
       v != eSIGNAL_IN_CUCC_MODE  &&
       v != eSIGNAL_IN_CTCC_MODE  &&
       v != eSIGNAL_IN_UNDEFIND_MODE)
        v = eSIGNAL_REG_FAIL;
 
      *type = t;
      *vendor =v;
      return 0;
  }


static int modem_get_signal_type(eMODEM_SIGNAL_TYPE *type,eMODEM_SIGNAL_VENDOR *vendor)
{
  int iret = -1;
  struct at_json *p_json = &at_json; 

  memset(tty_buf,0,MAX_TTY_LEN);

  if(modem_send_cmd_for_result(tty_fd,p_json->type,tty_buf,MAX_TTY_LEN)==0)
  {
    get_type_value(tty_buf,type,vendor);
    iret  = 0;
  }
 
  return iret;
}

#define VENDOR_DIV "\n\n"

static int get_vendor_value(char *vendor,sMODEM_VENDOR_INFO *info)
{
   char *temp = strtok(vendor,VENDOR_DIV);

   strncpy(info->vendor,temp,MAX_LEN);
   temp = strtok(NULL,VENDOR_DIV);
   strncpy(info->type,temp,MAX_LEN);
   temp = strtok(NULL,VENDOR_DIV);
   strncpy(info->version,temp,MAX_LEN);

   return 0;
 }

static int modem_get_vendor(sMODEM_VENDOR_INFO *info)
{
  int iret = -1;
  struct at_json *p_json = &at_json; 

  memset(tty_buf,0,MAX_TTY_LEN);

  if(modem_send_cmd_for_result(tty_fd,p_json->vendor,tty_buf,MAX_TTY_LEN)==0)
  {
    get_vendor_value(tty_buf,info);
    iret  = 0;
  }
 
  return iret;


}

static int modem_start_dhcpcd(void)
{
  int iret;

  iret = system("/etc/init.d/dhcpcd_modem start");

  if(iret)
  {
       MODEM_LOGE("system start hostapd error %d\n", errno);
  }

  return iret;
}

static void modem_stop_dhcpcd(void)
{
	int iret;

	iret = system("/etc/init.d/dhcpcd_modem stop");
	if (iret) 
	{
		MODEM_LOGE("system stop hostapd error %d\n", errno);
	}

    return;
}

static int modem_dev_open(const hw_module_t * module,
			const char *name __unused, hw_device_t ** device)
{
    int i, fd,iret= 0;
    struct at_json *p_json = &at_json;
    struct modemtty_device_t *modemdev = calloc(1, sizeof(modemtty_device_t));

    if (!modemdev) 
    {
	  MODEM_LOGE("Can not allocate memory for the leds device");
	  return -ENOMEM;
    }

    modem_set_bug_level();

    MODEM_LOGI("%s\n", __func__);

    modemdev->common.tag = HARDWARE_DEVICE_TAG;
    modemdev->common.module = (hw_module_t *) module;
    modemdev->common.version = HARDWARE_DEVICE_API_VERSION(1, 0);
    modemdev->dev_close = modem_dev_close;

    modemdev->on                  = modem_on; 
    modemdev->off                 = modem_off;
    modemdev->get_status           = modem_get_state;
    modemdev->get_signal_strength = modem_get_signal_strength;
    modemdev->get_signal_type     = modem_get_signal_type;
    modemdev->get_vendor          = modem_get_vendor;
    modemdev->start_dhcpcd          = modem_start_dhcpcd;
    modemdev->stop_dhcpcd           = modem_stop_dhcpcd;

    *device = (struct hw_device_t *)modemdev;

   if(json_parse_at_file(p_json)!= 0)
    {
      MODEM_LOGE("json parse at file fail\n");
      return -1;
    }

#ifdef MODEM_TTY_DEBUG
     DUMP_SRING(p_json,name);
     DUMP_SRING(p_json,state);
     DUMP_SRING(p_json,strength);
     DUMP_SRING(p_json,type);
     DUMP_SRING(p_json,vendor);
     DUMP_SRING(p_json,eth);
     
     printf("------------------------------\n"); 
     printf("        dump 'on' at cmds     \n");
     printf("------------------------------\n"); 
     for(i=0;i<p_json->on.total;i++)
        printf("cmd %d:%s\n",i,p_json->on.p[i]);
     printf("------------------------------\n"); 
     
     printf("------------------------------\n"); 
     printf("       dump 'off' at cmds     \n");
     printf("------------------------------\n"); 
     for(i=0;i<p_json->off.total;i++)
        printf("cmd %d:%s\n",i,p_json->off.p[i]);
     printf("------------------------------\n"); 
#endif
    
    if(modem_tty_init(&fd) !=0)
    {
        MODEM_LOGE("modem_tty_init fail\n");
        iret = -1;
    }

    tty_fd = fd;

    return iret;
}

static struct hw_module_methods_t modem_module_methods = 
{
    .open = modem_dev_open,
};

struct hw_module_t HAL_MODULE_INFO_SYM = 
{
    .tag = HARDWARE_MODULE_TAG,
    .module_api_version = MODEM_API_VERSION,
    .hal_api_version = HARDWARE_HAL_API_VERSION,
    .id = MODEM_TTY_HW_ID,
    .name = "ROKID MODEM HAL: The Coral solution.",
    .author = "The Linux Foundation",
    .methods = &modem_module_methods,
};
