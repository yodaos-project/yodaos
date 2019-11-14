#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <string.h>

#include <hardware/leds.h>
#include <hardware/hardware.h>
static void split(char *src,const char *separator,char **dest,int *num) 
{
    char *pNext;
     int count = 0;
      if (src == NULL || strlen(src) == 0)
         return;
      if (separator == NULL || strlen(separator) == 0)
         return;    
      pNext = strtok(src,separator);
      while(pNext != NULL) {
           *dest++ = pNext;
           ++count;
          pNext = strtok(NULL,separator);  
     }  
     *num = count;
}  

static inline int led_dev_open(const hw_module_t *module, struct ledarray_device_t **device) {
	return module->methods->open(module,LED_ARRAY_HW_ID,(struct hw_device_t **)device);
}

struct led_module_t *module = NULL;
struct ledarray_device_t *leds_device = NULL;

int main()
{
	int status;
	char szTest[10000] = {0};  
	char *buf[1000]={0}, *buf_0[1000]={0};
	unsigned int *uint_Frame_Buffer;
	unsigned char *LED_RGB_Buf;  
	char temp[64];
	int For_Num=0,Led_Count=0,Interval_Time=0, Frame_Count=0;
   	int intI = 0,Num=0;  
	
	status = hw_get_module( LED_ARRAY_HW_ID, (const struct hw_module_t **)&module);
	printf("leds HAL test begin.\n");
	if(status == 0) {
		printf("hw_get_module run successfully.\n");
		if(0 != led_dev_open(&module->common, &leds_device))
			return -1;
	}

	if (!module) {
		printf("leds HAL test error %d\n", status);
	}

 
	int fd = open("/data/Led_Config.txt", O_RDONLY);
	if (fd < 0)
	{
	printf("failed to open Led_Config.txt\n");  
	return -1;
   	}

	ssize_t ret = read(fd, szTest, sizeof(szTest));
    	if (ret < 0)  
	{
		printf("%s", strerror(errno));
		return -1;
    	}
	split(szTest, "\r\n", buf, &Num);
	//printf("1. %s\n%s\n%s\n%s\n%s\n--%d\n",buf[0],buf[1],buf[2],buf[3],buf[4],Num);
	close(fd);
	
	//split the frist line and get some params
	split(buf[0], ":", buf_0, &Num);	
	For_Num = atol(buf_0[0]);
	Led_Count = atol(buf_0[1]);
	Interval_Time = atol(buf_0[2]);
	Frame_Count = atol(buf_0[3]);
	//printf("2. %d-%d-%d-%d\n",For_Num,Led_Count,Interval_Time,Frame_Count);
	uint_Frame_Buffer = (unsigned int *)malloc(1024 * Led_Count * Frame_Count);
	LED_RGB_Buf = (unsigned char *)malloc(sizeof(unsigned char) * Led_Count * Frame_Count * 3);

	//Split the frame line to array
	for(int line_num=0;line_num<Frame_Count;line_num++)
	{
		split(buf[line_num+2], ",", buf_0, &Num);
		for (int led_num=0;led_num<Led_Count;led_num++)
		{
			//Get led frame color  -"RRGGBB"(str) to oxRRGGBB(十六进制数) 
			sprintf(temp, "0x%s", buf_0[led_num]);  
			int tt=strtol(temp, NULL, 16);
			uint_Frame_Buffer[led_num + Led_Count * line_num]=tt;
			//printf("3. %d -- %d -- %x\n",line_num,led_num,uint_Frame_Buffer[led_num + Led_Count * line_num]); 
		}
	}


	//get Led Frame RGB - color
	for (int xx=0;xx<Led_Count*Frame_Count;xx++)
	{
		//printf("%x-\n",long_Frame_Buffer[xx]);
		LED_RGB_Buf[xx * 3 + 0]=uint_Frame_Buffer[xx]>>16 & 0xFF;
		LED_RGB_Buf[xx * 3 + 1]=uint_Frame_Buffer[xx]>>8 & 0xFF;
		LED_RGB_Buf[xx * 3 + 2]=uint_Frame_Buffer[xx] & 0xFF;
		//printf("%x----%x,%x,%x\n",uint_Frame_Buffer[xx],LED_RGB_Buf[xx*3 + 0],LED_RGB_Buf[xx*3 + 1],LED_RGB_Buf[xx*3  + 2]);
	}

/*
	for (int xx=0;xx<Led_Count * Frame_Count * 3;xx=xx+3)
	{
		printf("4. %x,%x,%x\n",LED_RGB_Buf[xx],LED_RGB_Buf[xx+1],LED_RGB_Buf[xx+2]);
	}
*/	
	printf("************debug****************\n");

	for (int xx=0;xx<For_Num;xx++)
	{
		printf("%d\n",xx);
		for(int color=0;color < Frame_Count;color++)
		{

			leds_device->draw(leds_device,&LED_RGB_Buf[color*Led_Count *3], Led_Count*3);
			printf("%d--%d--%d\n",xx,color,Interval_Time);
			usleep(Interval_Time*1000);
		
		}

	}

	printf("leds HAL test end.\n");
	free(uint_Frame_Buffer);
	free(LED_RGB_Buf);
	return 0;
}
