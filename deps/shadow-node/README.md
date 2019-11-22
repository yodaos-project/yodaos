# ShadowNode

The Node.js runtime in shadow, enables N-API and vast Node.js packages on edge devices.

[![Build Status](https://travis-ci.org/yodaos-project/ShadowNode.svg?branch=master)](https://travis-ci.org/yodaos-project/ShadowNode)
[![License](https://img.shields.io/badge/licence-Apache%202.0-brightgreen.svg?style=flat)](LICENSE)

The project is another runtime for your [Node.js][] packages, while ShadowNode is designed to be used on memory limited devices. It's inspired and forked from the awesome project [Samsung/iotjs][].

## Quick Start

To get started with ShadowNode, you could download prebuilt binaries on [Release Page](https://github.com/yodaos-project/ShadowNode/releases) for following targets:

- Linux x64
- macOS x64

> Memory usage and binary footprint are measured at [here](https://samsung.github.io/js-remote-test) with real target daily.

## Documentation

- [Getting Started](docs/Getting-Started.md)
- [API Reference](docs/api/README.md)

### Build

##### Fetch source code
```sh
$ git clone https://github.com/yodaos-project/ShadowNode.git
$ cd ShadowNode
```

##### Build ShadowNode
```sh
$ npm run build
```

##### Get available build options
```sh
$ tools/build.py --help
```

##### Install
```sh
$ tools/build.py --install
```

##### Run tests
```sh
$ npm test
```

For additional information see [Getting Started](docs/Getting-Started.md).

## Compared with Node.js

ShadowNode is not designed to be ran identical code that ran on Node.js.
While edge environments are experiencing limited resources on runtime, the packages to be ran on ShadowNode shall be rewritten in a resource compact way. However for the very initial thought of sharing the Node.js vast module ecosystem, we would like make ShadowNode compatible with Node.js with our efforts.


- [Assert](docs/api/Assert.md)
- [Buffer](docs/api/Buffer.md)
- [C/C++ Addon API](docs/api/N-API.md)
- [Child Process](docs/api/Child-Process.md)
- [Crypto](docs/api/Crypto.md)
- [DNS](docs/api/DNS.md)
- [Events](docs/api/Events.md)
- [File System](docs/api/File-System.md)
- [HTTP](docs/api/HTTP.md)
- [Module](docs/api/Module.md)
- [Net](docs/api/Net.md)
- [OS](docs/api/OS.md)
- [Process](docs/api/Process.md)
- [Stream](docs/api/Stream.md)
- [Timers](docs/api/Timers.md)
- [TLS](docs/api/TLS.md)
- [UDP/Datagram](docs/api/DGRAM.md)
- [Zlib](docs/api/Zlib.md)

Since the `MQTT` protocol is commonly used for communication between IoT devices, ShadowNode supports
the protocol natively, and keeps the API consistent with the popular library [MQTT.js][]. See
[MQTT API](docs/api/MQTT.md) for details.

The `WebSocket` is a popular protocol in IoT environment as well, and also supported by [ShadowNode][]
natively. See [WebSocket API](docs/api/WebSocket.md).

For hardware geek, this project benefits from the upstream [IoT.js][], which has supported the
following hardware interfaces, you are able to port [ShadowNode][] to your platforms and
start hacking with JavaScript:

- [ADC](docs/api/ADC.md)
- [BLE](docs/api/BLE.md)
- [GPIO](docs/api/GPIO.md)
- [I2C](docs/api/I2C.md)
- [PWM](docs/api/PWM.md)
- [SPI](docs/api/SPI.md)
- [UART](docs/api/UART.md)

## License

[ShadowNode][] is Open Source software under the [Apache 2.0 license][].
Complete license and copyright information can be found within the code.

[ShadowNode]: https://github.com/yodaos-project/ShadowNode
[Node.js]: https://github.com/nodejs/node
[Iot.js]: https://github.com/Samsung/iotjs
[Samsung/iotjs]: https://github.com/Samsung/iotjs
[MQTT.js]: https://github.com/mqttjs/MQTT.js
[Apache 2.0 license]: https://www.apache.org/licenses/LICENSE-2.0
