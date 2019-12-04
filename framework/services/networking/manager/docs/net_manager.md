# rokid net_manager

## 软件接口协议

目前 IPC 通讯依赖于 flora 框架，建议使用 agent 的 flora 接口进行同步收发命令。对于 js 上层应用提供网络状态和网络控制接口。具体 ipc 通信通道（Channel）如下：

- 网络状态: **network.status**，用于上报网络状态。
- 网路命令: **network.command**，用于传输命令。

## 网络状态

网络状态由**network.status**主动上报。

网络状态列表：

```
Publish1: {
  "network": {
    "state": "CONNECTED|DISCONNECTED",
  }
}
```
```
Publish2: {
  "wifi": {
    "state": "CONNECTED|DISCONNECTED",
    "signal": "-40",
  }
}
```
```
Publish3: {
  "ethernet": {
    "state": "CONNECTED|DISCONNECTED",
  }
}
```
```
Publish4: {
  "modem": {
    "state": "CONNECTED|DISCONNECTED",
    "type": "4G",
    "signal": "-12",
  }
}
```


## 网络控制

控制命令使用declareMethod，fragment为net_manager，即unix:/var/run/flora.sock#net_manager

Response1:{"result": "OK", "具体信息"}
Response3:{"result":"NOK","reason":"Respond fail reason"}

Respond fail reason:
  "NONE",
  "WIFI_NOT_CONNECT",
  "WIFI_NOT_FOUND",
  "WIFI_WRONG_KEY",
  "WIFI_TIMEOUT",
  "WIFI_CFGNET_TIMEOUT",
  "NETWORK_OUT_OF_CAPACITY",
  "WIFI_WPA_SUPLICANT_ENABLE_FAIL",
  "WIFI_HOSTAPD_ENABLE_FAIL",
  "COMMAND_FORMAT_ERROR",
  "COMMAND_RESPOND_ERROR",
  "WIFI_MONITOR_ENABLE_FAIL",
  "WIFI_SSID_PSK_SET_FAIL"

##### 网络状态

触发网路状态上报：

```
Request: {
  "device": "NETWORK",
  "command": "TRIGGER_STATUS",
}

Response1: {"result": "OK"}
Response2:{"result":"NOK","reason":"Respond fail reason"}
```

获取网络状态:

```
Request: {
  "device": "NETWORK",
  "command": "GET_STATUS",
}

Response: {
  "result": "OK",
  "network": {
    "state": "CONNECTED|DISCONNECTED",
  }
}

```
Request: {
  "device": "WIFI",
  "command": "GET_STATUS",
}

Response: {
  "result": "OK",
  "wifi": {
    "state": "CONNECTED|DISCONNECTED",
    "signal": "-49",
  }
}

```

Request: {
  "device": "MODEM",
  "command": "GET_STATUS",
}

Response: {
  "result": "OK",
  "modem": {
    "state": "CONNECTED|DISCONNECTED",
    "typde": "4G"
    "signal": "-49",
  }
}

```
Request: {
  "device": "ETHERENT",
  "command": "GET_STATUS",
}

Response: {
  "result": "OK",
  "etherent": {
    "state": "CONNECTED|DISCONNECTED",
  }
}

```
##### 网路能力

- wifi
- ethernet
- modem

获取网路能力：

```
Request: {
  "device": "NETWORK",
  "command": "GET_CAPACITY",
}

Response1: {"result": "OK", "net_capacities": []}
Response2: {"result": "OK", "net_capacities": ["wifi"]}
Response3: {"result": "OK", "net_capacities": ["modem", "wifi"]}
Response4: {"result": "OK", "net_capacities": ["modem", "wifi", "ethernet"]}
```

##### 网路模式

- wifi
- wifi_ap
- ethernet
- modem


设置当前网路模式：

Request: {
  "device": "NETWORK",
  "command": "GET_MODE"
}

Response1: {"result": "OK", "net_modess": []}
Response2: {"result": "OK", "net_modes": ["wifi"]}
Response3: {"result": "OK", "net_modes": ["wifi","modem"]}
Response4: {"result": "OK", "net_modes": ["wifi","etherent"]}
Response5: {"result": "OK", "net_modes": ["wifi","modem","ethernet"]}
Response6: {"result": "OK", "net_modes": ["wifi_ap"]}
Response7: {"result": "OK", "net_modes": ["wifi_ap","modem"]}
Response8: {"result": "OK", "net_modes": ["wifi_ap","etherent"]}
Response9: {"result": "OK", "net_modes": ["wifi_ap","modem","ethernet"]}

获取当前网路模式：
```
Request: {
  "device": "WIFI",
  "command": "CONNECT|DISCONNECT",
  "params": {
  "SSID": "xxxx",
  "PASSWD": "xxxx"
  }
}

Response1: {"result": "OK"}
Response2:{"result":"NOK","reason":"Respond fail reason"}
```
```
Request: {
  "device": "WIFI_AP",
  "command": "CONNECT|DISCONNECT",
  "params": {
    "SSID": "xxxx",
    "PASSWD": "xxxx",
    "IP": "xxxx"
  }
}

Response1: {"result": "OK"}
Response2:{"result":"NOK","reason":"Respond fail reason"}
```
```
Request: {
  "device": "MODEM",
  "command": "CONNECT|DISCONNECT"
}

Response1:{"result": "OK",}
Response2:{"result":"NOK","reason":"Respond fail reason"}
```
```
Request: {
  "device": "ETHERNET",
  "command": "CONNECT|DISCONNECT"
}

Response1:{"result": "OK"}
Response2:{"result":"NOK","reason":"Respond fail reason"}
```

重置当前网络的DNS

```
Request:
{
   "device" : "NETWORK",
   "command": "RESET_DNS"
}

Response1:{"result": "OK"}
Response2:{"result":"NOK","reason":"Respond fail reason"}

##### 其他

启动wifi扫描

```
Request: {
  "device": "WIFI",
  "command": "START_SCAN"
}

Response1:{"result": "OK"}
Response2:{"result":"NOK","reason":"Respond fail reason"}
```

关闭wifi扫描

```
Request: {
  "device": "WIFI",
  "command": "STOP_SCAN"
}

Response1:{"result": "OK"}
Response2:{"result":"NOK","reason":"Respond fail reason"}
```

获取扫描wifi列表

```
Request: {
  "device": "WIFI",
  "command": "GET_WIFILIST"
}

Response: {
  "result": "OK",
  "wifilist": [
    {"SSID": "xxxx", "SIGNAL": "xxxx"},
    {"SSID": "xxxx", "SIGNAL": "xxxx"}
  ]
}

获取配置文件中配置数

```
Request:
{
  "device" : "WIFI",
  "command": "GET_CFG_NUM" 
}

Response:
{
  "result": "OK",
  "wifi_config": "xxxx"
}

保存配置到配置文件

```
Request:
{
   "device" : "WIFI",
   "command": "SAVE_CFG"
}

Response1:{"result": "OK"}
Response2:{"result":"NOK","reason":"Respond fail reason"}

移除配置文件中对应信息配置

```
Request:
{
   "device" : "WIFI",
   "command": "REMOVE"
  "params": {
  "SSID": "xxxx",
  "PASSWD": "xxxx"
  }
}

Response1:{"result": "OK"}
Response2:{"result":"NOK","reason":"Respond fail reason"}

移除配置文件中所有信息配置

```
Request:
{
  "device" : "WIFI",
  "command": "REMOVE_ALL"
}

Response1:{"result": "OK"}
Response2:{"result":"NOK","reason":"Respond fail reason"}

wifi enable  all network

```
Request:
{
   "device" : "WIFI",
   "command": "ENABLE"
}

Response1:{"result": "OK"}
Response2:{"result":"NOK","reason":"Respond fail reason"}

wifi disable  all network

```
Request:
{
   "device" : "WIFI",
   "command": "DISABLE"
}

Response1:{"result": "OK"}
Response2:{"result":"NOK","reason":"Respond fail reason"}
