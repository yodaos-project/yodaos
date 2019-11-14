
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#include <sys/syscall.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <pthread.h>

#include <hardware/mic_array.h>

static inline int mic_array_device_open (const hw_module_t *module, struct mic_array_device_t **device) {
	return module->methods->open (module, MIC_ARRAY_HARDWARE_MODULE_ID, (struct hw_device_t **) device);
}

struct mic_array_module_t* module = NULL;
struct mic_array_device_t* mic_array_device = NULL;

void main()
{
	int ret;
	int mic_array_frame_size;
	int read_size = 65535;

	char *Buff;
	FILE *fp = NULL;
	int frame = 5535;

	if (hw_get_module (MIC_ARRAY_HARDWARE_MODULE_ID, (const struct hw_module_t **)&module) == 0) {
		//open mic array
		if (0 != mic_array_device_open(&module->common, &mic_array_device)) {
			return;
		}
	}

	fp = fopen("/data/debug.pcm", "wb");


	printf("File open\n");
	mic_array_frame_size = mic_array_device->get_stream_buff_size(mic_array_device);
	printf("mic_array_frame_size=%d\n",mic_array_frame_size);

        /** mod by jiaming.shan@rokid for Raspberry pi 3 Model B+ **/
//	mic_array_device->start_stream(mic_array_device);
	if(mic_array_device->start_stream(mic_array_device))
        {
		printf("mic array start stream failed\n");
		return ; 
	}
	printf("start stream\n");
	Buff = (char *)malloc(read_size);
    
        /** add by jiaming.shan@rokid for Raspberry pi 3 Model B+ **/
        if(Buff ==NULL)
        {
            printf("#Err:malloc %d bytes failed!\n",read_size);
            return;        
        }        

	int total = 0;
	while(frame > 0){
                /** mod by jiaming.shan@rokid for Raspberry pi 3 Model B+ **/
		//ret = mic_array_device->read_stream(mic_array_device, Buff, &read_size);
		ret = mic_array_device->read_stream(mic_array_device, Buff,read_size);
		if(ret != 0) {
			printf("read stream failed\n");
                        break;
		} else {
			total += read_size;
			printf("read stream size is=%d, total=%d\n",read_size, total);
		}
                /** mod by jiaming.shan@rokid for Raspberry pi 3 Model B+ **/
		//fwrite (Buff, sizeof(char), 480*6*3, fp);
		fwrite (Buff, sizeof(char), read_size,fp);
		frame--;
	}
	fclose(fp);
       /** add by jiaming.shan@rokid for Raspberry pi 3 Model B+ **/
        free(Buff);
	mic_array_device->stop_stream(mic_array_device);
}
