### Battery Service

------

#### 概述

代码版本：v0.1

基于linux标准的Power Supply接口，并以Flora作为进程间通讯机制的电源管理服务。

flora说明：https://github.com/Rokid/yoda-flora/blob/master/README.md

Power Supply：https://www.kernel.org/doc/Documentation/power/power_supply_class.txt

#### 编译

需要首先确认平台是否配置battery service的package，配置选项名为：PACKAGE_battery_service=y

单独编译模块：make package/battery_service/clean,make package/battery_service/compile,make package/battery_service/install

#### 目录说明

- 源码目录：frameworks/native/services/battery_service/
- ./demo目录
  - bat_test.c : 基于flora模拟battery service可能发出的几类flora消息类型，比如高温，低温，充电状态变换，电量变化信息等，刷机后可以直接shell执行bat_test
  - bat_monitor.c : 基于flora的消息接收服务，主要为了测试battery service发出的flora消息是否合理，刷机后可以直接shell执行bat_monitor.
- ./include目录：battery service的头文件所在, battery_info.h  common.h  flora_bat.h
- ./src目录：battery service的源码目录: battery_monitor.c  CMakeLists.txt  flora_bat.c  main.c
  - battery_monitor.c : 封装了各种对底层power supply的信息的获取接口，这里可以很容易增加其他Power supply的属性信息
  - flora_bat.c：从battery monitor获取电池的各种属性信息，并根据mNeedSendMsg标志判断是否需要发送flora的信息，通知关心电源管理状态的其他应用和服务，这里可以很容易增加需要发送的
  - main.c ：battery service的主程序入口

#### 重要的结构体

- struct healthd_config 封装底层power supply接口的path路径，这里可以自由添减电池属性所在path路径

  ```c
   struct healthd_config {
       int periodic_chores_interval_fast;
       int periodic_chores_interval_slow;
  
       char batteryStatusPath[128];
       char batteryHealthPath[128];
       char batteryPresentPath[128];
       char batteryCapacityPath[128];
       char batteryVoltagePath[128];
       char batteryTemperaturePath[128];
       char batteryTechnologyPath[128];
       char batteryCurrentNowPath[128];
       char batteryChargeCounterPath[128];
   };
  ```

- struct BatteryProperties封装当前的电池信息和最近的一次电池信息，这里消息格式很多，有些是应用层不关心的属性信息，可以自由添减

```c
 struct BatteryProperties {
     bool chargerAcOnline;
     bool chargerUsbOnline;
     bool chargerWirelessOnline;
     int maxChargingCurrent;
     int maxChargingVoltage;
     int batteryStatus;
     int batteryHealth;
     bool batteryPresent;
     int batteryLevel;
     int batteryVoltage;
     int batteryTemperature;
     int batteryCurrent;
     int batteryCycleCount;
     int batteryFullCharge;
     int batteryChargeCounter;

     bool chargerAcOnlineLast;
     bool chargerUsbOnlineLast;
     bool chargerWirelessOnlineLast;
     int maxChargingCurrentLast;
     int maxChargingVoltageLast;
     int batteryStatusLast;
     int batteryHealthLast;
     bool batteryPresentLast;
     int batteryLevelLast;
     int batteryVoltageLast;
     int batteryTemperatureLast;
     int batteryCurrentLast;
     int batteryCycleCountLast;
     int batteryFullChargeLast;
     int batteryChargeCounterLast;

     char batteryTechnology[16];
 };
```

- mNeedSendMsg：判断是否需要发送flora消息的重要标志位。

#### flora消息格式说明

- 一个示例：

```c
json_object_object_add(root, "batChargingOnline", json_object_new_boolean(0));//truefalse
json_object_object_add(root, "batChargingCurrent", json_object_new_int(0)); // 单位mA
json_object_object_add(root, "batChargingVoltage", json_object_new_int(0)); //单位mV
json_object_object_add(root, "batChargingCounter", json_object_new_int(10)); //不处理
json_object_object_add(root, "batTimetoEmpty", json_object_new_int(315)); //单位分钟min
json_object_object_add(root, "batStatus", json_object_new_int(0)); // 充电与否
json_object_object_add(root, "batPresent", json_object_new_boolean(1)); //电池在否
json_object_object_add(root, "batLevel", json_object_new_int(80)); //电量百分比
json_object_object_add(root, "batVoltage", json_object_new_int(4100)); //电池电压mV
json_object_object_add(root, "batTemp", json_object_new_int(35)); //电池温度摄氏度
```
