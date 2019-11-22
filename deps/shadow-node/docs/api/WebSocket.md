# WebSocket

The WebSocket v13 client implementation module, which is compatible with 
[WebSocket-Node](https://github.com/theturtle32/WebSocket-Node).

**Example**

```js
var WebSocketClient = require('websocket').client;
var client = new WebSocketClient();

client.on('connectFailed', function(error) {
    console.log('Connect Error: ' + error.toString());
});

client.on('connect', function(connection) {
  console.log('WebSocket Client Connected');
  connection.on('error', function(error) {
    console.log("Connection Error: " + error.toString());
  });
  connection.on('close', function() {
    console.log('echo-protocol Connection Closed');
  });
  connection.on('message', function(message) {
    if (message.type === 'utf8') {
      console.log("Received: '" + message.utf8Data + "'");
    }
  });
  
  function sendNumber() {
    if (connection.connected) {
      var number = Math.round(Math.random() * 0xFFFFFF);
      connection.sendUTF(number.toString());
      setTimeout(sendNumber, 1000);
    }
  }
  sendNumber();
});

client.connect('ws://localhost:8080/', 'echo-protocol');
```

## Class: WebSocketClient

```js
var WebSocketClient = require('websocket').client;
```

### new WebSocketClient()

Creates a new `WebSocket` client.

### client.connect(url, protocol)

* `url` {String} the url to connect.
* `protocol` {String} the websocket sub protocol param.

### Event `'connect'`

* `connection` {WebSocketConnection} the returned connection object.

Emitted on successful (re)connection (i.e. connack rc=0).

### Event `'connectFailed'`

Emitted when a connection is failed.

## Class: WebSocketConnection

### connection.sendUTF(data)

* `data` {String|Buffer} the data to send as a text frame.

Send a text frame.

### connection.sendBytes(data)

* `data` {Buffer} the data to send as a binary frame.

Send a binary frame.

### Event `'message'`

* `frame` {Object} the received frame object.
  * `type` {String} `utf8` or `binary`.
  * `utf8Data` {String} the data when type is `utf8`.
  * `binaryData` {Buffer} the data when type is `binary`.

### Event `'close'`

Emitted when connection is close.

### Error `'error'`

* `err` {Error} the error.

Emitted when an error occurs.

## Client Example using the W3C WebSocket API

This module supported the W3C WebSocket Standard API, see the following:

```js
var W3CWebSocket = require('websocket').w3cwebsocket;
var client = new W3CWebSocket('ws://localhost:8080/', 'echo-protocol');

client.onerror = function() {
  console.log('Connection Error');
};

client.onopen = function() {
  console.log('WebSocket Client Connected');

  function sendNumber() {
    if (client.readyState === client.OPEN) {
      var number = Math.round(Math.random() * 0xFFFFFF);
      client.send(number.toString());
      setTimeout(sendNumber, 1000);
    }
  }
  sendNumber();
};

client.onclose = function() {
  console.log('echo-protocol Client Closed');
};

client.onmessage = function(e) {
  if (typeof e.data === 'string') {
    console.log("Received: '" + e.data + "'");
  }
};
```

For more detailed, please see [WebSocket-Node Readme](https://github.com/theturtle32/WebSocket-Node).
