### Power Manager Service

------

#### 概述

版本：v0.3

基于linux内核的autosleep和wakelock机制实现YodaOS系统级的电源管理，各进程之间的通信服务由flora机制提供，power manager service管理各应用的锁申请，最后向内核统一申请pm_service wakelock

autosleep和wakelock：https://lwn.net/Articles/479841/

flora说明：https://github.com/Rokid/yoda-flora/blob/master/README.md

#### 编译

需要首先确认平台是否配置powermanager service的package，配置选项名为：PACKAGE_powermanager_service=y

单独编译模块：

- make package/powermanager_service/clean

- make package/powermanager_service/compile

- make package/powermanager_service/install

#### 目录说明

- 源码目录：frameworks/native/services/powermanager_service/
- ./demo目录
  - pms_test.c : 基于flora模拟power manager service可能收到的几种请求：设定alarm，可以sleep，或者通过power manager service提供的wakelock服务向内核申请wakelock，从而阻止系统休眠，刷机后可以直接shell执行pms_test.
  - pms_monitor.c : 基于flora的消息接收服务，主要为了监听其他应用向power manager service发出的flora消息以及pms本身对flora消息的处理是否准确，用于调试目的，刷机后可以直接shell执行pms_monitor.
- ./include目录：powermanager service的头文件所在, common.h  flora_pm.h  power.h
- ./src目录：powermanager service的源码目录: flora_pm.c  main.c  power.c
  - power.c : 封装了封装了wakelock的request和release，以及sleep的各种接口，这里需要调用power HAL里提供的访问内核提供的wakelock/autosleep的接口
  - flora_pm.c：基于flora的消息处理机制，目前处理MSG_SLEEP,MSG_LOCK,MSG_ALARM,MSG_VOICE_COMMING四种消息，还在开发中，可能会增减其他消息类型的处理。
  - main.c ：powermanager service的主程序入口

#### 重要的接口说明

- wakelock和autosleep

```c
//full lock和partial lock为以后显示模块预留的wakelock的锁机制
//暂时只用full wakelock
extern int partial_lock_total;
extern int full_lock_total;

 enum {
         NO_LOCK_STATE = 0,
         PART_LOCK_STATE,
         FULL_LOCK_STATE,
 };

 int power_module_init(); //HAL层调用
 int power_wakelock_request(int lock, const char* id); //wakelock 申请
 int power_wakelock_release(const char* id); //wakelock 释放
 int power_autosleep_request(const char* id); //autosleep 申请
 int power_sleep_request(const char* id); // sleep 申请
 int power_lock_state_poll(); // wakelock 查询
```

- flora的消息使用

```c
     do {
         printf("power.wakelock setting:\n");
         printf("    1 pms_wakelock request\n");
         printf("    2 pms_wakelock release\n");

         printf("    99 exit \n");

         choice = app_get_choice("Select cmd");

         json_object *root = json_object_new_object();

         switch(choice) {
         case 1:
             command = "ON"; // wakelock 申请的消息处理
             json_object_object_add(root, "name", json_object_new_string(name));
             json_object_object_add(root, "wakelock", json_object_new_string(command));
             break;
         case 2:
             command = "OFF"; //wakelock 释放的消息处理
             json_object_object_add(root, "name", json_object_new_string(name));
             json_object_object_add(root, "wakelock", json_object_new_string(command));
             break;
         case 99:
             break;
         default:
             break;
         }

         json_object_object_add(root, "proto", json_object_new_string(proto));

         pm_command = json_object_to_json_string(root);

         caps_t msg = caps_create();
         caps_write_string(msg, (const char *)pm_command);

         flora_cli_post(cli, "power.wakelock", msg, FLORA_MSGTYPE_INSTANT);
         caps_destroy(msg);

     } while (choice != 99);
```
