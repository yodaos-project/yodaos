#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <stdbool.h>
#include <limits.h>
#include <errno.h>
#include <common.h>

#define POWER_SUPPLY_SUBSYSTEM "power_supply"
#define POWER_SUPPLY_SYSFS_PATH "/sys/class/" POWER_SUPPLY_SUBSYSTEM
#define CHARGER_LED_PATH "/sys/devices/platform/led-stage/charger_led_ctrl"
#define maxChargerNum 6
#define maxChargerNameSize 64
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

char mChargerNames[maxChargerNum][maxChargerNameSize];
int mChargerNum = 0;
bool mNeedSendMsg = true;
bool mLedPresent = true;
struct healthd_config* mHealthdConfig;
struct BatteryProperties props;

struct sysfsStringEnumMap {
    char* s;
    int val;
};

struct cvChargingTime {
    int level; //bat level
    int times; //mins
};

const static struct cvChargingTime chargeTime[] = {
        {86, 61},
        {87, 59},
        {88, 57},
        {89, 55},
        {90, 52},
        {91, 49},
        {92, 46},
        {93, 42},
        {94, 38},
        {95, 33},
        {96, 28},
        {97, 22},
        {98, 15},
        {99, 8},
        {100, 0},
};

static int mBatteryGetCvTimeByLevel(int level) {
    int i =0;
    for (i = 0; i < ARRAY_SIZE(chargeTime); i++) {
        if (chargeTime[i].level == level)
            return chargeTime[i].times;
    }
    return 0;
}

static int mapSysfsString(const char* str,
                          struct sysfsStringEnumMap map[]) {
    for (int i = 0; map[i].s; i++)
        if (!strcmp(str, map[i].s))
            return map[i].val;

    return -1;
}

size_t strlcat(dst, src, siz)
	char *dst;
	const char *src;
	size_t siz;
{
	register char *d = dst;
	register const char *s = src;
	register size_t n = siz;
	size_t dlen;

	/* Find the end of dst and adjust bytes left but don't go past end */
	while (n-- != 0 && *d != '\0')
		d++;
	dlen = d - dst;
	n = siz - dlen;

	if (n == 0)
		return(dlen + strlen(s));
	while (*s != '\0') {
		if (n != 1) {
			*d++ = *s;
			n--;
		}
		s++;
	}
	*d = '\0';

	return(dlen + (s - src));	/* count does not include NUL */
}

int getBatteryStatus(const char* status) {
    int ret;
    struct sysfsStringEnumMap batteryStatusMap[] = {
        { "Unknown", BATTERY_STATUS_UNKNOWN },
        { "Charging", BATTERY_STATUS_CHARGING },
        { "Discharging", BATTERY_STATUS_DISCHARGING },
        { "Not charging", BATTERY_STATUS_NOT_CHARGING },
        { "Full", BATTERY_STATUS_FULL },
        { NULL, 0 },
    };

    ret = mapSysfsString(status, batteryStatusMap);
    if (ret < 0) {
        KLOG_WARNING(LOG_TAG "Unknown battery status '%s'\n", status);
        ret = BATTERY_STATUS_UNKNOWN;
    }

    return ret;
}

int getBatteryHealth(const char* status) {
    int ret;
    struct sysfsStringEnumMap batteryHealthMap[] = {
        { "Unknown", BATTERY_HEALTH_UNKNOWN },
        { "Good", BATTERY_HEALTH_GOOD },
        { "Overheat", BATTERY_HEALTH_OVERHEAT },
        { "Dead", BATTERY_HEALTH_DEAD },
        { "Over voltage", BATTERY_HEALTH_OVER_VOLTAGE },
        { "Unspecified failure", BATTERY_HEALTH_UNSPECIFIED_FAILURE },
        { "Cold", BATTERY_HEALTH_COLD },
        { NULL, 0 },
    };

    ret = mapSysfsString(status, batteryHealthMap);
    if (ret < 0) {
        KLOG_WARNING(LOG_TAG "Unknown battery health '%s'\n", status);
        ret = BATTERY_HEALTH_UNKNOWN;
    }

    return ret;
}

int readFromFile(char* path, char* buf, size_t size) {
    char *cp = NULL;

    if (!path)
        return -1;
    int fd = open(path, O_RDONLY, 0);
    if (fd == -1) {
        KLOG_ERROR(LOG_TAG "Could not open '%s'\n", path);
        return -1;
    }

    ssize_t count = TEMP_FAILURE_RETRY(read(fd, buf, size));
    if (count > 0)
            cp = (char *)memrchr(buf, '\n', count);

    if (cp)
        *cp = '\0';
    else
        buf[0] = '\0';

    close(fd);
    return count;
}

enum PowerSupplyType readPowerSupplyType(char* path) {
    const int SIZE = 128;
    char buf[SIZE];
    int length = readFromFile(path, buf, SIZE);
    enum PowerSupplyType ret;
    struct sysfsStringEnumMap supplyTypeMap[] = {
            { "Unknown", ANDROID_POWER_SUPPLY_TYPE_UNKNOWN },
            { "Battery", ANDROID_POWER_SUPPLY_TYPE_BATTERY },
            { "UPS", ANDROID_POWER_SUPPLY_TYPE_AC },
            { "Mains", ANDROID_POWER_SUPPLY_TYPE_AC },
            { "USB", ANDROID_POWER_SUPPLY_TYPE_USB },
            { "USB_DCP", ANDROID_POWER_SUPPLY_TYPE_AC },
            { "USB_CDP", ANDROID_POWER_SUPPLY_TYPE_AC },
            { "USB_ACA", ANDROID_POWER_SUPPLY_TYPE_AC },
            { "Wireless", ANDROID_POWER_SUPPLY_TYPE_WIRELESS },
            { NULL, 0 },
    };

    if (length <= 0)
        return ANDROID_POWER_SUPPLY_TYPE_UNKNOWN;

    ret = (enum PowerSupplyType)mapSysfsString(buf, supplyTypeMap);
    if (ret < 0)
        ret = ANDROID_POWER_SUPPLY_TYPE_UNKNOWN;

    return ret;
}

bool getBooleanField(char* path) {
    const int SIZE = 16;
    char buf[SIZE];

    bool value = false;
    if (readFromFile(path, buf, SIZE) > 0) {
        if (buf[0] != '0') {
            value = true;
        }
    }

    return value;
}

int getIntField(char* path) {
    const int SIZE = 128;
    char buf[SIZE];

    int value = 0;
    if (readFromFile(path, buf, SIZE) > 0) {
        value = strtol(buf, NULL, 0);
    }
    return value;
}

int healthd_board_battery_update(struct BatteryProperties *props)
{
    // return 0 to log periodic polled battery status to kernel log
    return 0;
}

bool mBatteryPropertiesUpdate(void) {
    bool logthis;

    props.chargerAcOnline = false;
    props.chargerUsbOnline = false;
    props.chargerWirelessOnline = false;
    props.batteryStatus = BATTERY_STATUS_UNKNOWN;
    props.batteryHealth = BATTERY_HEALTH_UNKNOWN;
    props.batteryCurrent = INT_MIN;
    props.batteryChargeCounter = INT_MIN;

    if (strcmp(mHealthdConfig->batteryPresentPath, ""))
        props.batteryPresent = getBooleanField(mHealthdConfig->batteryPresentPath);
    else
        props.batteryPresent = true;

    if (strcmp(mHealthdConfig->batteryCapacityPath, ""))
        props.batteryLevel = getIntField(mHealthdConfig->batteryCapacityPath);

    if (strcmp(mHealthdConfig->batteryVoltagePath, ""))
        props.batteryVoltage = \
		getIntField(mHealthdConfig->batteryVoltagePath) / 1000;

    if (strcmp(mHealthdConfig->batteryCurrentNowPath, ""))
        props.batteryCurrent = getIntField(mHealthdConfig->batteryCurrentNowPath);

    if (strcmp(mHealthdConfig->batteryChargingCurrentPath, ""))
        props.batteryChargingCurrent = \
		getIntField(mHealthdConfig->batteryChargingCurrentPath) / 1000;

    if (strcmp(mHealthdConfig->batteryChargeCounterPath, ""))
        props.batteryChargeCounter = \
		getIntField(mHealthdConfig->batteryChargeCounterPath);

    if (strcmp(mHealthdConfig->batteryTemperaturePath, ""))
        props.batteryTemperature = getIntField(mHealthdConfig->batteryTemperaturePath);

    const int SIZE = 128;
    char buf[SIZE];

    if (strcmp(mHealthdConfig->batteryStatusPath, "") &&
	(readFromFile(mHealthdConfig->batteryStatusPath, buf, SIZE) > 0))
        props.batteryStatus = getBatteryStatus(buf);

    if (strcmp(mHealthdConfig->batteryHealthPath, "") &&
	(readFromFile(mHealthdConfig->batteryHealthPath, buf, SIZE) > 0))
        props.batteryHealth = getBatteryHealth(buf);
#if 0
    if (readFromFile(mHealthdConfig->batteryTechnologyPath, buf, SIZE) > 0)
        snprintf(props.batteryTechnology, 16, buf);
#endif
    unsigned int i;

    for (i = 0; i < mChargerNum; i++) {
        char path[SIZE];

        snprintf(path, SIZE, "%s/%s/online", POWER_SUPPLY_SYSFS_PATH,
                          mChargerNames[i]);
        if (readFromFile(path, buf, SIZE) > 0) {
            if (buf[0] != '0') {
                strcpy(path, "");
                snprintf(path, SIZE,"%s/%s/type", POWER_SUPPLY_SYSFS_PATH,
                                  mChargerNames[i]);
                switch(readPowerSupplyType(path)) {
                case ANDROID_POWER_SUPPLY_TYPE_AC:
                    props.chargerAcOnline = true;
                    break;
                case ANDROID_POWER_SUPPLY_TYPE_USB:
                    props.chargerUsbOnline = true;
                    break;
                case ANDROID_POWER_SUPPLY_TYPE_WIRELESS:
                    props.chargerWirelessOnline = true;
                    break;
                default:
                    KLOG_WARNING(LOG_TAG "%s: Unknown power supply type\n",
                                 mChargerNames[i]);
                }
            }
        }
    }

    logthis = !healthd_board_battery_update(&props);

    if (logthis) {
        char dmesgline[256];
        snprintf(dmesgline, sizeof(dmesgline),
                 "battery l=%d v=%d t=%s%d.%d h=%d st=%d cc=%d",
                 props.batteryLevel, props.batteryVoltage,
                 props.batteryTemperature < 0 ? "-" : "",
                 abs(props.batteryTemperature / 10),
                 abs(props.batteryTemperature % 10), props.batteryHealth,
                 props.batteryStatus, props.batteryChargingCurrent);

        if (strcmp(mHealthdConfig->batteryCurrentNowPath, "")) {
            char b[20];

            snprintf(b, sizeof(b), " c=%d", props.batteryCurrent / 1000);
            strlcat(dmesgline, b, sizeof(dmesgline));
        }
#ifdef BAT_DEBUG
        KLOG_INFO(LOG_TAG "%s chg=%s%s%s\n", dmesgline,
                  props.chargerAcOnline ? "a" : "",
                  props.chargerUsbOnline ? "u" : "",
                  props.chargerWirelessOnline ? "w" : "");
#endif
    }

    return props.chargerAcOnline | props.chargerUsbOnline |
            props.chargerWirelessOnline;
}

void mBatteryUseTimeReport(struct BatteryProperties* bp) {

    int fullEnergy = 3800; //mAh
    int cvChargingBegin = 85;
    int cvCostTime = 61; //mins

    if (bp->batteryLevel < 5) {
	bp->batteryTimetoEmpty = -1;
	bp->batterySleepTimetoEmpty = -1;
    } else {
	bp->batteryTimetoEmpty \
	    = fullEnergy * (bp->batteryLevel -5) * 60 / (450 * 100); //mins
	bp->batterySleepTimetoEmpty \
	    = fullEnergy * (bp->batteryLevel -5) * 60 / (18 * 100); //hours
    }

    if (!(bp->chargerAcOnline | bp->chargerUsbOnline | bp->chargerWirelessOnline))
	bp->batteryTimetoFull = -1;
    else if (bp->batteryLevel <= cvChargingBegin \
	     && bp->batteryChargingCurrent > 0 )
	bp->batteryTimetoFull = (cvChargingBegin - bp->batteryLevel) * \
			    fullEnergy * 60 / (bp->batteryChargingCurrent * 100)
			    + cvCostTime;
    else if (bp->batteryLevel > cvChargingBegin \
	     && bp->batteryLevel <= 100 \
	     && bp->batteryChargingCurrent > 0)
        bp->batteryTimetoFull = mBatteryGetCvTimeByLevel(bp->batteryLevel);
    else
	bp->batteryTimetoFull = -1;

}

int mBatteryWriteChargerLeds(struct BatteryProperties* bp) {
    const int SIZE = 128;
    char path[128];
    char buf[2];
    int fd;

    if (!mLedPresent)
	return 0;

    snprintf(path, SIZE, "%s", CHARGER_LED_PATH);
    fd = open(path, O_RDWR, 0666);
    if (fd == -1) {
	mLedPresent = false;
	KLOG_ERROR(LOG_TAG "failed to open charger led path\n");
	return -1;
    }

    if (bp->batteryLevel < 20)
	snprintf(buf, 2, "%d", LED_CHARGER_LOW);
    else if (bp->chargerAcOnline | bp->chargerUsbOnline \
	       | bp->chargerWirelessOnline) {
	if (bp->batteryLevel == 100)
            snprintf(buf, 2, "%d", LED_CHARGER_FULL);
	else
	    snprintf(buf, 2, "%d", LED_CHARGER_ON);
    } else
	snprintf(buf, 2, "%d", LED_CHARGER_OFF);
#ifdef BAT_DEBUG
    KLOG_INFO(LOG_TAG "led buf %s\n",buf);
#endif
    write(fd, buf, 2);
    close(fd);

    return 0;
}

static bool mBatCapacityDecrease = false;
int mBatteryLowPowerCheck(struct BatteryProperties* bp) {
    const int capAlert = 6;
    const int curAlert = 30; //30mA
    const int retryCount = 15; //times
    static int count = 0;

    if (bp->chargerAcOnline | bp->chargerUsbOnline | bp->chargerWirelessOnline) {

        if ((bp->batteryLevel < capAlert) && (bp->batteryLevel > 0)) {
	    if (mBatCapacityDecrease) {
                KLOG_ERROR(LOG_TAG "mBatteryLowPowerCheck:rokid_reboot:l=%d\n",
			   bp->batteryLevel);
		system("rokid_reboot charging");
	    }

	} else if (bp->batteryLevel == 0) {

	    if (bp->batteryChargingCurrent < curAlert) {
		if(++count > retryCount) {
                    KLOG_ERROR(LOG_TAG "mBatteryLowPowerCheck:rokid_reboot:c<30ma\n");
		    system("rokid_reboot charging");
		}
	    } else
		count = 0;

        }

     }

    return 0;
}

bool mLastBatteryPropertiesUpdate(struct BatteryProperties* bp) {

    if (bp->chargerAcOnlineLast != bp->chargerAcOnline) {
        bp->chargerAcOnlineLast = bp->chargerAcOnline;
	mNeedSendMsg = true;
    }

    if (bp->chargerUsbOnlineLast != bp->chargerUsbOnline) {
        bp->chargerUsbOnlineLast = bp->chargerUsbOnline;
	mNeedSendMsg = true;
    }

    if (bp->chargerWirelessOnlineLast != bp->chargerWirelessOnline) {
        bp->chargerWirelessOnlineLast = bp->chargerWirelessOnline;
	mNeedSendMsg = true;
    }

    if (bp->maxChargingCurrentLast != bp->maxChargingCurrent) {
        bp->maxChargingCurrentLast = bp->maxChargingCurrent;
	//mNeedSendMsg = true;
    }

    if (bp->maxChargingVoltageLast != bp->maxChargingVoltage) {
        bp->maxChargingVoltageLast = bp->maxChargingVoltage;
	//mNeedSendMsg = true;
    }

    if (bp->batteryStatusLast != bp->batteryStatus) {
        bp->batteryStatusLast = bp->batteryStatus;
	mNeedSendMsg = true;
    }

    if (bp->batteryHealthLast != bp->batteryHealth) {
        bp->batteryHealthLast = bp->batteryHealth;
	mNeedSendMsg = true;
    }

    if (bp->batteryPresentLast != bp->batteryPresent) {
        bp->batteryPresentLast = bp->batteryPresent;
	mNeedSendMsg = true;
    }

    if (bp->batteryLevelLast != bp->batteryLevel) {

	if (bp->batteryLevelLast > bp->batteryLevel)
	    mBatCapacityDecrease = true;
	else
	    mBatCapacityDecrease = false;

        bp->batteryLevelLast = bp->batteryLevel;
	mNeedSendMsg = true;
    }

    if (bp->batteryVoltageLast != bp->batteryVoltage) {
        bp->batteryVoltageLast = bp->batteryVoltage;
	//mNeedSendMsg = true;
    }

    if (bp->batteryTemperatureLast != bp->batteryTemperature) {
        bp->batteryTemperatureLast = bp->batteryTemperature;
	mNeedSendMsg = true;
    }

    if (bp->batteryCurrentLast != bp->batteryCurrent) {
        bp->batteryCurrentLast = bp->batteryCurrent;
	//mNeedSendMsg = true;
    }

    if (bp->batteryCycleCountLast != bp->batteryCycleCount) {
        bp->batteryCycleCountLast = bp->batteryCycleCount;
	//mNeedSendMsg = true;
    }

    if (bp->batteryFullChargeLast != bp->batteryFullCharge) {
        bp->batteryFullChargeLast = bp->batteryFullCharge;
	mNeedSendMsg = true;
    }

    if (bp->batteryChargeCounterLast != bp->batteryChargeCounter) {
        bp->batteryChargeCounterLast = bp->batteryChargeCounter;
	//mNeedSendMsg = true;
    }

    return mNeedSendMsg;
}

void mHealthdConfigUpdate(struct healthd_config *hc) {
    const int SIZE = 128;
    char path[SIZE];

    DIR* dir = opendir(POWER_SUPPLY_SYSFS_PATH);
    if (dir == NULL) {
        KLOG_ERROR(LOG_TAG "Could not open %s\n", POWER_SUPPLY_SYSFS_PATH);
    } else {
        struct dirent* entry;

        while ((entry = readdir(dir))) {
            const char* name = entry->d_name;

            if (!strcmp(name, ".") || !strcmp(name, ".."))
                continue;

            // Look for "type" file in each subdirectory
            strcpy(path, "");
            snprintf(path, SIZE, "%s/%s/type", POWER_SUPPLY_SYSFS_PATH, name);
            switch(readPowerSupplyType(path)) {
            case ANDROID_POWER_SUPPLY_TYPE_AC:
            case ANDROID_POWER_SUPPLY_TYPE_USB:
            case ANDROID_POWER_SUPPLY_TYPE_WIRELESS:
                if (strcmp(mHealthdConfig->batteryChargingCurrentPath, "") == 0) {
                    strcpy(path, "");
                    snprintf(path, SIZE, "%s/%s/constant_charge_current",
			     POWER_SUPPLY_SYSFS_PATH, name);
                    if (access(path, R_OK) == 0)
                        strcpy(mHealthdConfig->batteryChargingCurrentPath, path);
                }

                if (strcmp(mHealthdConfig->batteryTemperaturePath, "") == 0) {
                    strcpy(path, "");
                    snprintf(path, SIZE, "%s/%s/temp", POWER_SUPPLY_SYSFS_PATH,
                                      name);
		    KLOG_INFO(LOG_TAG "battery temp path: %s\n", name);
                    if (access(path, R_OK) == 0) {
                        strcpy(mHealthdConfig->batteryTemperaturePath, path);
                    }
		}

                strcpy(path, "");
                snprintf(path, SIZE, "%s/%s/online", POWER_SUPPLY_SYSFS_PATH, name);
                if (access(path, R_OK) == 0)
                    strcpy(mChargerNames[mChargerNum], name);
		KLOG_INFO(LOG_TAG "%dth ChargerName: %s\n", mChargerNum,
			  mChargerNames[mChargerNum]);
		mChargerNum++;
                break;

            case ANDROID_POWER_SUPPLY_TYPE_BATTERY:
                if (strcmp(mHealthdConfig->batteryStatusPath, "") == 0) {
                    strcpy(path, "");
                    snprintf(path, SIZE, "%s/%s/status", POWER_SUPPLY_SYSFS_PATH,
                                      name);
                    if (access(path, R_OK) == 0)
                        strcpy(mHealthdConfig->batteryStatusPath, path);
                }

                if (strcmp(mHealthdConfig->batteryHealthPath, "") == 0) {
                    strcpy(path, "");
                    snprintf(path, SIZE, "%s/%s/health", POWER_SUPPLY_SYSFS_PATH,
                                      name);
                    if (access(path, R_OK) == 0)
                        strcpy(mHealthdConfig->batteryHealthPath, path);
                }

                if (strcmp(mHealthdConfig->batteryPresentPath, "") == 0) {
                    strcpy(path, "");
                    snprintf(path, SIZE, "%s/%s/present", POWER_SUPPLY_SYSFS_PATH,
                                      name);
                    if (access(path, R_OK) == 0)
                        strcpy(mHealthdConfig->batteryPresentPath, path);
                }

                if (strcmp(mHealthdConfig->batteryCapacityPath, "") == 0) {
                    strcpy(path, "");
                    snprintf(path, SIZE, "%s/%s/capacity", POWER_SUPPLY_SYSFS_PATH,
                                      name);
                    if (access(path, R_OK) == 0)
                        strcpy(mHealthdConfig->batteryCapacityPath, path);
                }

                if (strcmp(mHealthdConfig->batteryVoltagePath, "") == 0) {
                    strcpy(path, "");
                    snprintf(path, SIZE, "%s/%s/voltage_now",
                                      POWER_SUPPLY_SYSFS_PATH, name);
                    if (access(path, R_OK) == 0) {
                        strcpy(mHealthdConfig->batteryVoltagePath, path);
                    } else {
                        strcpy(path, "");
                        snprintf(path, SIZE, "%s/%s/batt_vol",
                                          POWER_SUPPLY_SYSFS_PATH, name);
                        if (access(path, R_OK) == 0)
                            strcpy(mHealthdConfig->batteryVoltagePath,  path);
                    }
                }

                if (strcmp(mHealthdConfig->batteryCurrentNowPath, "") == 0) {
                    strcpy(path, "");
                    snprintf(path, SIZE, "%s/%s/current_now",
                                      POWER_SUPPLY_SYSFS_PATH, name);
                    if (access(path, R_OK) == 0)
                        strcpy(mHealthdConfig->batteryCurrentNowPath, path);
                }

                if (strcmp(mHealthdConfig->batteryChargeCounterPath, "") == 0) {
                    strcpy(path, "");
                    snprintf(path, SIZE, "%s/%s/charge_counter",
                                      POWER_SUPPLY_SYSFS_PATH, name);
                    if (access(path, R_OK) == 0)
                        strcpy(mHealthdConfig->batteryChargeCounterPath, path);
                }
                if (strcmp(mHealthdConfig->batteryTemperaturePath, "") == 0) {
                    strcpy(path, "");
                    snprintf(path, SIZE, "%s/%s/temp", POWER_SUPPLY_SYSFS_PATH,
                                      name);
                    if (access(path, R_OK) == 0) {
                        strcpy(mHealthdConfig->batteryTemperaturePath, path);
                    } else {
                        strcpy(path, "");
                        snprintf(path, SIZE, "%s/%s/batt_temp",
                                          POWER_SUPPLY_SYSFS_PATH, name);
                        if (access(path, R_OK) == 0)
                            strcpy(mHealthdConfig->batteryTemperaturePath, path);
                    }
                }

                if (strcmp(mHealthdConfig->batteryTechnologyPath, "") == 0) {
                    strcpy(path, "");
                    snprintf(path, SIZE, "%s/%s/technology",
                                      POWER_SUPPLY_SYSFS_PATH, name);
                    if (access(path, R_OK) == 0)
                        strcpy(mHealthdConfig->batteryTechnologyPath, path);
                }

                break;

            case ANDROID_POWER_SUPPLY_TYPE_UNKNOWN:
                break;
            }
        }
        closedir(dir);
    }

    if (!mChargerNum)
        KLOG_ERROR(LOG_TAG "No charger supplies found\n");
    if (strcmp(mHealthdConfig->batteryStatusPath, "") == 0)
        KLOG_WARNING(LOG_TAG "BatteryStatusPath not found\n");
    if (strcmp(mHealthdConfig->batteryHealthPath, "") == 0)
        KLOG_WARNING(LOG_TAG "BatteryHealthPath not found\n");
    if (strcmp(mHealthdConfig->batteryPresentPath, "") == 0)
        KLOG_WARNING(LOG_TAG "BatteryPresentPath not found\n");
    if (strcmp(mHealthdConfig->batteryCapacityPath, "") == 0)
        KLOG_WARNING(LOG_TAG "BatteryCapacityPath not found\n");
    if (strcmp(mHealthdConfig->batteryVoltagePath, "") == 0)
        KLOG_WARNING(LOG_TAG "BatteryVoltagePath not found\n");
    if (strcmp(mHealthdConfig->batteryTemperaturePath, "") == 0)
        KLOG_WARNING(LOG_TAG "BatteryTemperaturePath not found\n");
    if (strcmp(mHealthdConfig->batteryTechnologyPath, "") == 0)
        KLOG_WARNING(LOG_TAG "BatteryTechnologyPath not found\n");
    if (strcmp(mHealthdConfig->batteryChargingCurrentPath, "") == 0)
        KLOG_WARNING(LOG_TAG "batteryChargingCurrentPath not found\n");

}

int mHealthdConfigInit() {

    struct healthd_config* hc = malloc(sizeof(struct healthd_config));
    if (!hc) {
	KLOG_ERROR(LOG_TAG "Error, memory not allocated \n");
	return -1;
    }

    hc->batteryStatusPath[0] = '\0';
    hc->batteryHealthPath[0] = '\0';
    hc->batteryPresentPath[0] = '\0';
    hc->batteryCapacityPath[0] = '\0';
    hc->batteryVoltagePath[0] = '\0';
    hc->batteryTemperaturePath[0] = '\0';
    hc->batteryTechnologyPath[0] = '\0';
    hc->batteryCurrentNowPath[0] = '\0';
    hc->batteryChargeCounterPath[0] = '\0';
    hc->batteryChargingCurrentPath[0] = '\0';

    mHealthdConfig = hc;

    return 0;
}

