#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <strings.h>
#include <memory.h>
#include <stdbool.h>
#include "src/volumecontrol.h"

int main(int argc, char** argv)
{
	int vol ,i;
	if(argc < 2 || argc > 5)
	{
		printf("need one param at least\n");
		return -1;
	}

	 if(strcmp(argv[1],"loop") == 0)
	{
		for(i=0;i<atoi(argv[2]);i++)
		{
			printf("the %d time begin\n",i);
			rk_set_all_volume(i);
			vol = rk_get_all_volume();
			printf("get all volume:%d\n",vol);
			sleep(1);
//			set_stream_volume(1,i);
//			vol = get_stream_volume(1);
 //      		printf("get tts  volume:%d\n",vol);
		}
		return 0;
	}

	if(strcmp(argv[1],"rk_is_mute") == 0){
		vol = (int)rk_is_mute();		
		printf("all mute is:%d\n", vol);
	}
	else if(strcmp(argv[1],"rk_is_stream_mute") == 0){
		vol = (int)rk_is_stream_mute(atoi(argv[2]));
		printf(" %s mute is:%d\n",argv[2],vol);
	} else if(strcmp(argv[1],"getappvolume") == 0){
		//vol = get_app_volume(argv[2]);
		printf("get %s volume:%d\n",argv[1],vol);
	} else if(strcmp(argv[1],"rk_set_stream_volume") == 0) {
		rk_set_stream_volume(atoi(argv[2]),atoi(argv[3]));
	} else if (strcmp(argv[1],"rk_get_stream_volume") == 0){
		vol = rk_get_stream_volume(atoi(argv[2]));
		printf("get %s volume:%d\n",argv[1],vol);
	} else if (strcmp(argv[1],"rk_set_all_volume") == 0) {
		rk_set_all_volume(atoi(argv[2]));
	} else if (strcmp(argv[1],"rk_get_all_volume") == 0) {
		vol = rk_get_all_volume();
		printf("get all volume:%d\n",vol);
	} else if (strcmp(argv[1],"rk_set_volume") == 0) {
		rk_set_volume(atoi(argv[2]));
	} else if (strcmp(argv[1],"rk_get_volume") == 0) {
		vol = rk_get_volume();
		printf("get volume:%d\n",vol);
	} else if(strcmp(argv[1],"rk_set_mute") == 0) {
		rk_set_mute(atoi(argv[2]));
	} else if(strcmp(argv[1],"rk_set_stream_mute") == 0) {
		rk_set_stream_mute(atoi(argv[2]),atoi(argv[3]));
    } else if(strcmp(argv[1],"rk_get_stream_playing_status") == 0) {
        int result = rk_get_stream_playing_status(atoi(argv[2]));
        printf("Stream type %d %s\n", atoi(argv[2]), result == 1 ? "is connected." :"isn't connected or the stream type is invalid!");
	} else if(strcmp(argv[1], "rk_initCustomVolumeCurve") == 0) {
        int result = rk_initCustomVolumeCurve();
        if (!result)
            printf("Initialize volume curve successfully!\n");
        else
            printf("Failed to set volume curve!\n");
    } else {
		printf("%s {rk_set_stream_volume <streamtye> <value> | rk_get_stream_volume <streamtype> | rk_is_stream_mute <streamtype> \n\
					rk_set_volume <value> | rk_get_volume value | rk_set_mute <value>}\n\
					rk_set_all_volume <value> | rk_get_all_volume <value> | rk_is_all_mute | rk_get_stream_playing_status <value> | rk_initCustomVolumeCurve\n"
			, argv[0]);
	}

	return 0;
}
