# TLS/SSL

ShadowNode provides asynchronous TLS/SSL networking module. You can use this module with `require('tls')` and create clients.

### tls.connect(options[, connectListener])

* `options` {Object} An object which specifies the connection options.
* `connectListener` {Function} Listener for the `'connect'` event.
* Returns {tls.TLSSocket}.

Creates a new `tls.TLSSocket` and automatically connects with the supplied `options`.
The `options` object specifies the following information:

* `ca` {String}the CA certification.
* `cert` {String} the client certification.
* `key` {String} the key for `cert`.
* `rejectUnauthorized` {Boolean} optional, default is `false`.

Others extend from `net.Socket`'s options.

**Example**

```js

var tls = require('tls');
var port = 443;

var secureSocket = tls.connect({
  port: port, 
  host: 'www.google.com'
}, function() {
  secureSocket.end('GET / HTTP/1.1\r\n\r\n');
});

var response = '';
secureSocket.on('data', function(data) {
  response += data;
});

secureSocket.on('end', function() {
  console.log(response);
});
```

## Class: tls.TLSSocket

This object is an abstraction of a TLS socket.

### socket.destroy()

Ensures that no more I/O activity happens on the socket and destroys the socket as soon as possible.

### socket.end([data][, callback])

* `data` {Buffer|string}
* `callback` {Function}

Half-closes the socket. The socket is no longer writable.
If `data` is given it is equivalent to `socket.write(data)` followed by `socket.end()`.

### socket.write(data[, callback])

* `data` {Buffer|String} Data to write.
* `callback` {Function} Executed function (when the data is finally written out).

Sends `data` on the socket.

The optional `callback` function will be called after the given data is flushed through the connection.

### Event: 'connect'

* `callback` {Function}

Emitted after connection is established.

### Event: 'close'

* `callback` {Function}

Emitted when the socket has been closed.

### Event: 'data'

* `callback` {Function}
  * `data` {Buffer} the decrypted buffer.

**Example**
```js

var tls = require('tls');
var socket = tls.connect({host: 'www.google.com', port: 443});
var msg = '';

/* ... */

socket.on('data', function(data) {
  msg += data;
});
```

### Event: 'end'

* `callback` {Function}

Emitted when FIN packet received.

### Event: 'error'

* `callback` {Function}
  * `err` {Error}

Emitted when an error occurs.
