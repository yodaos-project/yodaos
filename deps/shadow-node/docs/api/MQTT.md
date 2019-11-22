# MQTT

This module is a native module for the `MQTT` protocol, which is compatible with
the [MQTT.js](https://github.com/mqttjs/MQTT.js).

## Example

```js
var mqtt = require('mqtt');
var client  = mqtt.connect('mqtt://test.mosquitto.org');

client.on('connect', function () {
  client.subscribe('presence');
  client.publish('presence', 'Hello mqtt');
});

client.on('message', function (topic, message) {
  // message is Buffer
  console.log(message.toString());
  client.end();
});
```

## Class: MqttClient

### new MqttClient(endpoint, options)

* `endpoint` {String} The uri of what your MQTT service at.
* `options` {Object} The options:
  * `keepalive` {Number} 60 seconds, set to 0 to disable
  * `clientId` {String} `'mqttjs_' + Math.random().toString(16).substr(2, 8)`
  * `protocolId` {String} `MQTT`
  * `protocolVersion` {String} `4`
  * `clean` {Boolean} `true`, set to false to receive QoS 1 and 2 messages while offline
  * `reconnectPeriod` {Number} `1000` milliseconds, interval between two reconnections
  * `connectTimeout` {Number} `30 * 1000` milliseconds, time to wait before a CONNACK is received
  * `username` {String} the username required by your broker, if any
  * `password` {String} the password required by your broker, if any

Creates a new client of `endpoint`.

### client.connect()

Connects the specified before url by MQTT.

### Event `'connect'`

Emitted on successful (re)connection (i.e. connack rc=0).

### Event `'reconnect'`

Emitted when a reconnect starts.

### Event `'offline'`

Emitted when the client goes offline.

### Event `'error'`

* `err` {Error} the error returned.

Emitted when the client cannot connect (i.e. connack rc != 0) or when a parsing error occurs.

### Event `'message'`

* `topic` {String} topic of the received packet.
* `message` {String} payload of the received packet.

Emitted when the client receives a publish packet.

### Event `'packetsend'`

Emitted when the client sends any packet. This includes .published() packets as well as packets used by MQTT for managing subscriptions and connections.

### Event `'packetreceive'`

Emitted when the client receives any packet. This includes packets from subscribed topics as well as packets used by MQTT for managing subscriptions and connections.

### client.publish(topic, message[, options[, callback]])

* `topic` {String} topic to publish
* `message` {String|Buffer} message to publish
* `options` {Object} options to publish with, including:
  * `qos` {Number} QoS level, default `0`
  * `dup` {Boolean} mark as duplicate flag, default `false`
  * `retain` {Boolean} retain flag, default `false`
* `callback` fired when the QoS handling completes, or at the next tick if QoS 0. An error occurs if client is disconnecting.

Publish a message to a topic.

### client.subscribe(topic[, options[, callback]])

* `topic` {String} topic to publish
* `options` {Object} options to publish with, including:
  * `qos` {Number} QoS level, default `0`
* `callback` callback fired on suback.

Subscribe to a topic.

> Not support on topics array or object.

### client.unsubscribe(topic[, callback])

* `topic` {String} topic to publish
* `callback` callback fired on suback.

Unsubscribe from a topic.

### client.end([force[, callback]])

* `force` {Boolean} passing it to true will close the client right away.
* `callback` {Function} will be called when the client is closed. This parameter is optional.

Close the client.

### client.disconnect([err, [, callback]])

This method is *deprecated*.

* `err` {Error} throw an error before disconnect. This parameter is optional.
* `callback` {Function} will be called when the client is closed. This parameter is optional.

Disconnect the client right away The `client` can safety be unref after call this method.

### client.reconnect()

Disconnect current connection(if connected) and connect again using the same options as connect().

For more detailed, please see [MQTT.js Readme](https://github.com/mqttjs/MQTT.js#client).
