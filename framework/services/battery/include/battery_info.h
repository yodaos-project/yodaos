#ifndef __BATTERY_INFO_H
#define __BATTERY_INFO_H

#include <stdbool.h>
#include <sys/syslog.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KLOG_ERROR	printf
#define KLOG_WARNING	printf
#define KLOG_INFO	printf
#define LOG_TAG		"battery_service: "

enum PowerSupplyType {
    ANDROID_POWER_SUPPLY_TYPE_UNKNOWN = 0,
    ANDROID_POWER_SUPPLY_TYPE_AC,
    ANDROID_POWER_SUPPLY_TYPE_USB,
    ANDROID_POWER_SUPPLY_TYPE_WIRELESS,
    ANDROID_POWER_SUPPLY_TYPE_BATTERY
};

enum {
    BATTERY_STATUS_UNKNOWN = 1,
    BATTERY_STATUS_CHARGING = 2,
    BATTERY_STATUS_DISCHARGING = 3,
    BATTERY_STATUS_NOT_CHARGING = 4,
    BATTERY_STATUS_FULL = 5,
};

enum {
    BATTERY_HEALTH_UNKNOWN = 1,
    BATTERY_HEALTH_GOOD = 2,
    BATTERY_HEALTH_OVERHEAT = 3,
    BATTERY_HEALTH_DEAD = 4,
    BATTERY_HEALTH_OVER_VOLTAGE = 5,
    BATTERY_HEALTH_UNSPECIFIED_FAILURE = 6,
    BATTERY_HEALTH_COLD = 7,
};

enum {
    BATTERY_PROP_CHARGE_COUNTER = 1, // equals BATTERY_PROPERTY_CHARGE_COUNTER
    BATTERY_PROP_CURRENT_NOW = 2, // equals BATTERY_PROPERTY_CURRENT_NOW
    BATTERY_PROP_CURRENT_AVG = 3, // equals BATTERY_PROPERTY_CURRENT_AVERAGE
    BATTERY_PROP_CAPACITY = 4, // equals BATTERY_PROPERTY_CAPACITY
    BATTERY_PROP_ENERGY_COUNTER = 5, // equals BATTERY_PROPERTY_ENERGY_COUNTER
    BATTERY_PROP_BATTERY_STATUS = 6, // equals BATTERY_PROPERTY_BATTERY_STATUS
};

enum {
    LED_CHARGER_OFF = 0,
    LED_CHARGER_LOW,
    LED_CHARGER_ON,
    LED_CHARGER_FULL,
};

enum {
    BOOT_MODE_NORMAL = 0,
    BOOT_MODE_CHARGING,
};

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

    int batteryChargingCurrent;
    int batteryTimetoFull;
    int batteryTimetoEmpty;
    int batterySleepTimetoEmpty;
    char batteryTechnology[16];
};

// periodic_chores_interval_fast, periodic_chores_interval_slow: intervals at
// which healthd wakes up to poll health state and perform periodic chores,
// in units of seconds:
//
//    periodic_chores_interval_fast is used while the device is not in
//    suspend, or in suspend and connected to a charger (to watch for battery
//    overheat due to charging).  The default value is 60 (1 minute).  Value
//    -1 turns off periodic chores (and wakeups) in these conditions.
//
//    periodic_chores_interval_slow is used when the device is in suspend and
//    not connected to a charger (to watch for a battery drained to zero
//    remaining capacity).  The default value is 600 (10 minutes).  Value -1
//    tuns off periodic chores (and wakeups) in these conditions.
//
// power_supply sysfs attribute file paths.  Set these to specific paths
// to use for the associated battery parameters.  healthd will search for
// appropriate power_supply attribute files to use for any paths left empty:
//
//    batteryStatusPath: charging status (POWER_SUPPLY_PROP_STATUS)
//    batteryHealthPath: battery health (POWER_SUPPLY_PROP_HEALTH)
//    batteryPresentPath: battery present (POWER_SUPPLY_PROP_PRESENT)
//    batteryCapacityPath: remaining capacity (POWER_SUPPLY_PROP_CAPACITY)
//    batteryVoltagePath: battery voltage (POWER_SUPPLY_PROP_VOLTAGE_NOW)
//    batteryTemperaturePath: battery temperature (POWER_SUPPLY_PROP_TEMP)
//    batteryTechnologyPath: battery technology (POWER_SUPPLY_PROP_TECHNOLOGY)
//    batteryCurrentNowPath: battery current (POWER_SUPPLY_PROP_CURRENT_NOW)
//    batteryChargeCounterPath: battery accumulated charge
//                                         (POWER_SUPPLY_PROP_CHARGE_COUNTER)

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
    char batteryChargingCurrentPath[128];
};

extern bool mNeedSendMsg;
extern struct healthd_config* mHealthdConfig;
extern struct BatteryProperties props;
extern int mChargerNum;

bool mBatteryPropertiesUpdate(void);
bool mLastBatteryPropertiesUpdate(struct BatteryProperties* bp);
void mBatteryUseTimeReport(struct BatteryProperties* bp);
void mHealthdConfigUpdate(struct healthd_config *hc);
int mBatteryWriteChargerLeds(struct BatteryProperties* bp);
int mBatteryLowPowerCheck(struct BatteryProperties* bp);
int mHealthdConfigInit();

void *mPowerOffChargingPoll(void *arg);
int mBootModeUpdate (void);

#ifdef __cplusplus
}
#endif

#endif
