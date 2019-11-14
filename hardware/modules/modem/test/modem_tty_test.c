#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <getopt.h>
#include <hardware/modem_tty.h>
#include <hardware/hardware.h>

struct modem_module_t *module = NULL;
struct modemtty_device_t *modem_device = NULL;


static inline int modem_dev_open(const hw_module_t *module, struct modemtty_device_t **device) 
{
	return module->methods->open(module,MODEM_TTY_HW_ID,(struct hw_device_t **)device);
}

static struct option long_options[] = {
{"on"                  , no_argument,  0, 'O'  }, /*65 = 'A'*/ /**uint8_t **/
{"off"                 , no_argument,  0, 'F'  },
{"get_status"           , no_argument,  0, 'S'  },
{"get_vendor"          , no_argument,  0, 'V'  },
{"get_signal_type"     , no_argument,  0, 'I'  },
{"get_signal_strength" , no_argument,  0, 'E'  },
{"start_dhcpcd"          , no_argument,  0, 'A'  },
{"stop_dhcpcd"           , no_argument,  0, 'P'  },
{"help"                , no_argument,  0, 'H'},
{0,                    0,          0,  0 }
};

int help_usage(void)
{
	int i = 0;

  printf("Usage:\n");
  printf("short option -%s\n","hHOFSVIEAP");

  while(long_options[i].name)
  {
	printf("long option --%s\n",long_options[i].name);
    i++; 	
  }
	return 0;
}

int modem_test(int argc,char *argv[])
{
	 int c;
	 int iret = -1;
	 int option_index = 0;

    int signal;
    eMODEM_SIGNAL_TYPE type;
    eMODEM_SIGNAL_VENDOR vendor;
    sMODEM_VENDOR_INFO info;

   if(argc <= 1)
     help_usage();

	 while(1)
	 {
       c = getopt_long(argc, argv,"hHOFSVIEAP",long_options, &option_index);

	    if(c == -1)
		{
			 return 0;
		}

		 switch (c)
		 {
			 case 'O':
             iret = modem_device->on();
			 break;
			 case 'F':
             iret=modem_device->off();
			 break;
			 case 'S':
             iret = modem_device->get_status(); 
             printf("state:%s\n",iret ==0? "Card OK":"Card Fail");
			 break;
			 case 'V':
             iret = modem_device->get_vendor(&info);
             printf("%s\n%s\n%s\n",info.vendor,info.type,info.version);
			 break;
			 case 'I':
             if((iret = modem_device->get_signal_type(&type,&vendor))==0)
             {
                 printf("type:%d,vendor:%d\n",(int)type,(int)vendor);
             }
			 break;
			 case 'E':
             if((iret = modem_device->get_signal_strength(&signal))==0)
             {
                printf("signal:%d\n",(int)signal);
             }
			 break;
			 case 'A':
             iret = modem_device->start_dhcpcd();
			 break;
			 case 'P':
             modem_device->stop_dhcpcd();
             iret = 0;
			 break;
             case 'h':
		     case 'H':
              iret = help_usage();

		   case '?':
			   break;

			 default:
			  printf("?? getopt returned character code 0%o ??\n", c);
		 }
	 }
   return iret;
}



int main(int argc,char * argv[])
{
	int status;

	printf("modem HAL test begin.\n");

	if(hw_get_module( MODEM_TTY_HW_ID,(const struct hw_module_t **)&module))
    {
		fprintf(stderr,"hw_get_module run fail\n");
        return -1;
    }

   if(modem_dev_open(&module->common, &modem_device))
   { 
	  fprintf(stderr,"modem_dev_open fail\n");
	  return -1;
   }

   modem_test(argc,argv);
   return 0;
}
