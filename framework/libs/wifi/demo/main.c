#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../wpa_command.h"

static struct wifi_network g_config = {
  .id = 0,
  .ssid = "ROKID.TC",
  .psk = "rokidguys",
  .key_mgmt = WIFI_KEY_WPA2PSK,
};


static void show_help(void)
{
  fprintf(stdout, "usage: scan/scan_r/join_network/wifi/ap\n");
  fprintf(stdout, "<command> :help\n");
}

int main(int argc, char **argv)
{
  int ret = -1;
  struct wifi_scan_list list;
  int i;

  if (argc > 1) {
    if (!strcmp(argv[1], "-h")) {
        show_help();
    } else if (!strcmp(argv[1], "scan")) {
        ret = wifi_scan();
      if (ret < 0) {
        printf("%s,wifi_scan error\n", __func__);
      }
    } else if (!strcmp(argv[1], "scan_r")) {
      memset(&list, 0, sizeof(struct wifi_scan_list));
      ret = wifi_get_scan_result(&list);
      if (ret < 0) {
        printf("%s,wifi_get_scan_result error\n", __func__);
      }
      for (i = 0; i < list.num; i++) {
        printf("ssid :: %s  psk :: %d\n", list.ssid[i].ssid, list.ssid[i].key_mgmt);
      }
    } else if (!strcmp(argv[1], "join_network")) {
      memset(&list, 0, sizeof(struct wifi_scan_list));
      ret = wifi_join_network(&g_config);
      if (ret < 0) {
        printf("%s,wifi_join_network error\n", __func__);
      }
  }else if(!strcmp(argv[1],"wifi")){
    int iret = 0;
    iret = wifi_change_ap_to_station();
    printf("wifi_change_ap_to_station() = %d\n",iret);
   }else if(!strcmp(argv[1],"ap")){
     int iret = 0;
     iret =  wifi_change_station_to_ap("shanjiamg","","192.168.1.2");
     printf("wifi_change_station_to_ap() = %d\n",iret);
  }else {
      show_help();
    }
}else{
      show_help();
  }

    return 0;
}
