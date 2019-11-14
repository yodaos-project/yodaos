#!/bin/sh

cmd=$1
txt=$2
py=$3

read -r line < /var/run/dbus/session
export $line

usage() {
cat <<USAGE
debug.sh [function [args...]]

    add -- 添加自定义激活词, 需要两个参数，激活词和拼音
    rm -- 删除自定义激活词，需要一个参数，就是要删除的激活词
    nlp -- 执行一个nlp，arg是一个json object字符串，包含asr和nlp，和可选的action
    asr -- 执行一个asr，activation会解析这个asr并执行，同时只会解析一个asr
    gc -- 执行一次内存垃圾回收
    key -- 模拟按键事件，需要一个参数，就是数值格式按键事件，没有参数时打印支持的按键事件
    notifymaster -- 通知手机app设备已在线
    wifi -- 模拟底层wifi状态变更事件 -1:unknown 0:INIVATE 1:SCANING 2:CONNECTED 3:UNCONNECTED 4:NET_SERVER_CONNECTED 5:NETSERVER_UNCONNECTED
    hib -- 休眠，关闭所有app
    light -- 切换灯光效果，需要一个参数表示灯光方法
    clean -- 清除启动时间戳，重启服务时会像正常启动一样有灯光和声音提示
    battery -- 模拟电源和电池事件 json字符串 格式{
    "status":"Unknown"(or "Charging" or "Discharging" or "Not charging" or "Full"),
    "charge_online":0(0未充电 1充电),
    "charge_current":0(充电电流),
    "temp":0(当前温度*10 例如27度则temp为270),
    "capacity":0(当前电量 范围0-100),
    "health":"Good"(默认Good),
    "battery_present":1(电池是否存在 1存在),
    "voltage_now":0(电池电压uv)  
}
    help -- 打印当前使用说明
USAGE
}

dbussend() {
	dbus-send "/activation/debug" "com.rokid.activation.debug.$1" "string:$2"
}

list_all_keys() {
cat <<ALL_KEYS
EVENT_POWER_OFF = 1
EVENT_RESET_NETWORK = 2
EVENT_PICKUP = 3
EVENT_VOLUME_UP = 4
EVENT_VOLUME_DOWN = 5
EVENT_HIBERNATE = 6
EVENT_MEDIA_PAUSE = 7
EVENT_POWER_START = 8
EVENT_POWER_END = 9
EVENT_RESTORE_FACTORY_SETTINGS = 10
EVENT_OPEN_BLUETOOTH = 11
EVENT_DEEP_HIBERNATE = 12
ALL_KEYS
}

if [[ -z "$cmd" ]]
then
    usage
elif [ "$cmd" == "add" ]
then
	dbussend "addvt" "{\"txt\":\"$txt\",\"py\":\"$py\"}"
elif [ "$cmd" == "rm" ]
then
	dbussend "rmvt" "$txt"
elif [ "$cmd" == "key" ]
then
    if [[ -z $txt ]]; then
        list_all_keys
    else
        dbussend $cmd $txt
    fi
elif [ "$cmd" == "help" ]
then usage
else
    if [[ -z $txt ]]; then
        dbussend $cmd ""
    else
        dbussend $cmd $txt
    fi
fi
