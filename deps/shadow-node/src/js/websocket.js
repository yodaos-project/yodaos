'use strict';

var http = require('http');
var util = require('util');
var url = require('url');
var EventEmitter = require('events').EventEmitter;

var FrameType = {
  INCOMPLETE: 0x00,
  TEXT: 0x01,
  BINARY: 0x02,
  OPENING: 0x05,
  CLOSING: 0x08,
  PING: 0x09,
  PONG: 0x0a,
  ERROR: 0x0f,
};

var ParseErrorType = {
  OK: 0,
  INVALID: -1,
  INCOMPLETE: -2,
};

/**
 * @class WebSocketConnection
 * @param {HttpClient} httpConnection
 * @param {WebSocketClient} wsClient
 */
function WebSocketConnection(httpConnection, wsClient) {
  EventEmitter.call(this);
  this.client = wsClient;
  this.socket = httpConnection.connection;
  this.socket.on('data', this.onsocketdata.bind(this));
  this.connected = true;
  this.lastMessage = null;
  this.lastChunk = null;

  this.on('ping', function() {
    var buf = native.encodeFrame(FrameType.PONG, new Buffer('PONG'));
    this.socket.write(buf);
  }.bind(this));
}
util.inherits(WebSocketConnection, EventEmitter);

/**
 * @method onsocketdata
 * @param {Buffer} chunk
 */
WebSocketConnection.prototype.onsocketdata = function(chunk) {
  if (this.lastChunk) {
    chunk = Buffer.concat([this.lastChunk, chunk]);
    this.lastChunk = null;
  }
  var decoded = native.decodeFrame(chunk);
  var errCode = decoded.err_code;
  if (errCode === ParseErrorType.INCOMPLETE) {
    this.lastChunk = chunk;
    return;
  } else if (errCode !== ParseErrorType.OK) {
    return;
  }
  var totalLen = decoded.total_len;
  var leftChunk;
  if (totalLen < chunk.byteLength) {
    leftChunk = chunk.slice(totalLen, chunk.byteLength);
  }
  do {
    if (decoded.fin === 0) {
      if (this.lastMessage && decoded.type === FrameType.INCOMPLETE) {
        this.lastMessage.chunks.push(decoded.buffer);
      } else {
        this.lastMessage = decoded;
        this.lastMessage.chunks = [ decoded.buffer ];
        delete this.lastMessage.buffer;
      }
      break;
    }
    if (decoded.type === FrameType.INCOMPLETE) {
      if (!this.lastMessage) {
        break;
      }
      this.lastMessage.chunks.push(decoded.buffer);
      this.lastMessage.buffer = Buffer.concat(this.lastMessage.chunks);
      delete this.lastMessage.chunks;
      decoded = this.lastMessage;
      this.lastMessage = null;
    }
    if (decoded.type === FrameType.PING) {
      this.emit('ping');
    } else if (decoded.type === FrameType.PONG) {
      this.emit('pong');
    } else if (decoded.type === FrameType.TEXT) {
      var message;
      try {
        message = decoded.buffer.toString('utf8');
      } catch (err) {
        this.emit('error', new Error('invalid websocket utf8 message'));
        break;
      }
      this.emit('message', {
        type: 'utf8',
        utf8Data: message,
      });
    } else if (decoded.type === FrameType.BINARY) {
      this.emit('message', {
        type: 'binary',
        binaryData: decoded.buffer,
      });
    } else {
      var err = new Error(`unhandled message type ${decoded.type}`);
      this.emit('error', err);
    }
  } while (false);
  if (leftChunk) {
    this.onsocketdata(leftChunk);
  }
};

/**
 * @method sendUTF
 * @param {Buffer} buffer
 */
WebSocketConnection.prototype.sendUTF = function(buffer) {
  if (!(buffer instanceof Buffer))
    buffer = new Buffer(buffer);
  var buf = native.encodeFrame(FrameType.TEXT, buffer);
  this.socket.write(buf);
};

/**
 * @method sendBytes
 * @param {Buffer} buffer
 */
WebSocketConnection.prototype.sendBytes = function(buffer) {
  if (!(buffer instanceof Buffer))
    buffer = new Buffer(buffer);
  var buf = native.encodeFrame(FrameType.BINARY, buffer);
  this.socket.write(buf);
};

/**
 * @method sendBytes
 * @param {Buffer} buffer
 */
WebSocketConnection.prototype.close = function(code, reason) {
  // TODO: code and reason
  this.socket.end();
};

/**
 * @class WebSocketClient
 */
function WebSocketClient(options) {
  EventEmitter.call(this);
  this._options = options || {};
  if (this._options.tlsOptions) {
    this._tlsOptions = this._options.tlsOptions;
  }
  this.connection = null;
  this.connected = false;
  this.wsKey = 'dGhlIHNhbXBsZSBub25jZQ==';  // mock for now
}
util.inherits(WebSocketClient, EventEmitter);

/**
 * @method handshake
 * @param {String} location
 * @param {String} subProtocol
 */
WebSocketClient.prototype.handshake = function(location, subProtocol) {
  if (!subProtocol)
    subProtocol = 'chat';
  if (Array.isArray(subProtocol))
    subProtocol = subProtocol.join(',');

  var self = this;
  var options = Object.assign({
    headers: {
      'Upgrade': 'websocket',
      'Connection': 'Upgrade',
      'Sec-WebSocket-Key': this.wsKey,
      'Sec-WebSocket-Protocol': subProtocol,
      'Sec-WebSocket-Version': '13'
    }
  }, url.parse(location));

  var request = http.get;
  if (options.protocol === 'wss:' ||
    options.protocol === 'https:') {
    request = require('https').get;
    if (this._tlsOptions) {
      options = Object.assign(this._tlsOptions, options);
    }
  }

  var httpConnection = request(options, function(request) {
    var headers = request.headers;
    if (request.statusCode !== 101 ||
      headers.upgrade !== 'websocket' ||
      !headers['sec-websocket-accept'] ||
      headers['sec-websocket-protocol'] !== subProtocol) {
      httpConnection.end();

      var msg = `handshake failed ${JSON.stringify(headers)}`;
      self.emit('connectFailed', new Error(msg));
    } else {
      self.connection = new WebSocketConnection(httpConnection, self);
      self.emit('connect', self.connection);
      self.connected = true;
    }
  });
  httpConnection.on('error', function(err) {
    self.emit('error', err);
  });
  httpConnection.on('end', function() {
    self.emit('close');
  });
};

/**
 * @method connect
 * @param {String} location
 * @param {String} subProtocol
 */
WebSocketClient.prototype.connect = function(location, subProtocol) {
  var self = this;
  self.handshake(location, subProtocol);
};

/**
 * @class W3CWebSocket
 */
function W3CWebSocket(location, subProtocol) {
  this._before = [];
  this._wsClient = new WebSocketClient();
  this._wsClient.connect(location, subProtocol);
  this._wsClient.once('connect', function() {
    this.dispatchEventMethod('open', []);
    this.passthroughEvent('error');
    this.passthroughEvent('close');
    this.passthroughEvent('message', 'message', function(frame) {
      return [{
        // MessageEvent
        data: frame.utf8Data || frame.binaryData
      }];
    });
    this._clearBefore();
  }.bind(this));
  this._wsClient.once('connectFailed', function(err) {
    throw err;
  });
}

W3CWebSocket.prototype.passthroughEvent = function(from, to, converter) {
  var self = this;
  to = to || from;
  self._wsClient.connection.on(from, function() {
    var args = arguments;
    if (typeof converter === 'function') {
      args = converter.apply(null, args);
    }
    self.dispatchEventMethod(to, args);
  });
};

W3CWebSocket.prototype.dispatchEventMethod = function(name, args) {
  name = `on${name}`;
  if (typeof this[name] === 'function') {
    this[name].apply(this, args);
  } else if (name === 'error') {
    throw args[0];
  }
};

W3CWebSocket.prototype.send = function(data) {
  if (!this._wsClient.connected) {
    this._before.push(data);
  } else {
    this._send(data);
  }
};

W3CWebSocket.prototype._send = function(data) {
  if (data instanceof Buffer) {
    return this._wsClient.connection.sendBytes(data);
  } else {
    return this._wsClient.connection.sendUTF(data);
  }
};

W3CWebSocket.prototype._clearBefore = function() {
  for (var i = 0; i < this._before.length; i++) {
    this._send(this._before[i]);
  }
  this._before.length = 0;
  delete this._before;
};

exports.client = WebSocketClient;
exports.w3cwebsocket = W3CWebSocket;
