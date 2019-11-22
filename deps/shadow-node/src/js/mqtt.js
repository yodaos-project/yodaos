'use strict';

var util = require('util');
var URL = require('url');
var net = require('net');
var EventEmitter = require('events').EventEmitter;

/*eslint-disable */
var MQTT_CONNECT = 1;
var MQTT_CONNACK = 2;
var MQTT_PUBLISH = 3;
var MQTT_PUBACK = 4;
var MQTT_PUBREC = 5;
var MQTT_PUBREL = 6;
var MQTT_PUBCOMP = 7;
var MQTT_SUBSCRIBE = 8;
var MQTT_SUBACK = 9;
var MQTT_UNSUBSCRIBE = 10;
var MQTT_UNSUBACK = 11;
var MQTT_PINGREQ = 12;
var MQTT_PINGRESP = 13;
var MQTT_DISCONNECT = 14;
/* eslint-enable */

var MAX_PACKET_ID = 0xffff;

function noop() {}

/**
 * @class MqttClient
 * @param {String} endpoint
 * @param {Object} options
 */
function MqttClient(endpoint, options) {
  EventEmitter.call(this);
  var obj = URL.parse(endpoint);
  this._host = obj.hostname;
  this._port = Number(obj.port) || 8883;
  this._protocol = obj.protocol;
  this._options = Object.assign({
    username: null,
    password: null,
    clientId: 'mqttjs_' + Math.random().toString(16).substr(2, 8),
    will: null,
    keepalive: 60,
    reconnectPeriod: 5000,
    connectTimeout: 30 * 1000,
    resubscribe: true,
    protocolId: 'MQTT',
    protocolVersion: 4,
    pingReqTimeout: 10 * 1000,
  }, options);

  // handle `options.will.payload` to be a Buffer
  if (this._options.will && this._options.will.payload) {
    var willMessage = this._options.will.payload;
    if (!Buffer.isBuffer(willMessage)) {
      this._options.will.payload = new Buffer(willMessage || '');
    }
  }
  this._isSocketConnected = false;
  this._isConnected = false;
  this._reconnectTimer = null;
  this._packetId = 1;
  this._keepAliveTimer = null;
  this._keepAliveTimeout = null;
  this._disconnected = false;
  this._handle = new native.MqttHandle(this._options);
  this._connectTimer = null;
  this._disconnectCb = null;
  Object.defineProperty(this, 'connected', {
    get: function() {
      return this._isConnected;
    },
  });

  Object.defineProperty(this, 'reconnecting', {
    get: function() {
      return !!this._reconnectTimer;
    },
  });

}
util.inherits(MqttClient, EventEmitter);

/**
 * @method connect
 */
MqttClient.prototype.connect = function() {
  this._disconnected = false;
  var timeout = this._options.connectTimeout;
  if (timeout > 0) {
    this._connectTimer = setTimeout(this._onConnectTimeout.bind(this), timeout);
  }
  var tls;
  var opts = Object.assign({
    port: this._port,
    host: this._host,
  }, this._options);
  if (this._protocol === 'mqtts:') {
    tls = require('tls');
    this._socket = tls.connect(opts, this._onconnect.bind(this));
  } else {
    this._socket = net.connect(opts, this._onconnect.bind(this));
  }
  this._socket.on('data', this._ondata.bind(this));
  this._socket.once('error', this._onerror.bind(this));
  this._socket.once('close', this._onclose.bind(this));
  this._lastChunk = null;
  return this;
};

MqttClient.prototype._onConnectTimeout = function() {
  this._closeConnection(new Error('connect timeout'));
};

/**
 * @method _onconnect
 */
MqttClient.prototype._onconnect = function() {
  this._isSocketConnected = true;
  var buf;
  try {
    buf = this._handle._getConnect();
  } catch (err) {
    this._closeConnection(err);
    return;
  }
  this._write(buf);
};

MqttClient.prototype._onerror = function(err) {
  this.emit('error', err);
};

MqttClient.prototype._onclose = function() {
  this._clearKeepAlive();
  if (this._connectTimer) {
    clearTimeout(this._connectTimer);
    this._connectTimer = null;
  }
  if (!this._disconnected && this._options.reconnectPeriod > 0) {
    var self = this;
    this._reconnectTimer = setTimeout(function() {
      self.reconnect();
    }, this._options.reconnectPeriod);
  }
  this._isSocketConnected = false;
  if (this._isConnected) {
    this._isConnected = false;
    this.emit('offline');
  }
  this.emit('close');
  if (this._disconnected) {
    if (this._disconnectCb) {
      var cb = this._disconnectCb;
      this._disconnectCb = null;
      cb();
    }
  }
};

MqttClient.prototype._ondata = function(chunk) {
  // one packet in multi chunks
  if (this._lastChunk) {
    chunk = Buffer.concat([this._lastChunk, chunk]);
    this._lastChunk = null;
  }
  var res;
  try {
    res = this._handle._readPacket(chunk);
  } catch (err) {
    this._closeConnection(err);
    return;
  }
  this.emit('packetreceive');

  if (res.type === MQTT_CONNACK) {
    if (this._connectTimer) {
      clearTimeout(this._connectTimer);
      this._connectTimer = null;
    }
    this._isConnected = true;
    this._keepAlive();
    this.emit('connect');
  } else if (res.type === MQTT_PUBLISH) {
    var msg;
    try {
      msg = this._handle._deserialize(chunk);
    } catch (err) {
      this._closeConnection(err);
      return;
    }
    if (msg.payloadMissingSize > 0) {
      this._lastChunk = chunk;
    } else {
      this.emit('message', msg.topic, msg.payload);
      if (msg.qos > 0) {
        // send publish ack
        try {
          var ack = this._handle._getAck(msg.id, msg.qos);
          this._write(ack);
        } catch (err) {
          this._closeConnection(err);
          return;
        }
      }
      // multi packets in one chunk
      if (msg.payloadMissingSize < 0) {
        var end = chunk.byteLength;
        var start = end + msg.payloadMissingSize;
        var leftChunk = chunk.slice(start, end);
        this._ondata(leftChunk);
      }
    }
  } else if (res.type === MQTT_PINGRESP) {
    this._onKeepAlive();
  } else {
    // FIXME handle other message type
    this.emit('unhandledMessage', res);
  }
};

/**
 * @method _write
 * @param {Buffer} buffer
 * @param {Function} callback
 */
MqttClient.prototype._write = function(buffer, callback) {
  var self = this;
  callback = callback || noop;
  if (!self._isSocketConnected) {
    callback(new Error('mqtt is not connected'));
    return;
  }
  self._socket.write(buffer, function() {
    self.emit('packetsend');
    callback();
  });
};

/**
 * @method _keepAlive
 */
MqttClient.prototype._keepAlive = function() {
  var self = this;
  if (self._options.keepalive === 0) {
    // set to 0 to disable
    return;
  }
  self._keepAliveTimer = setTimeout(function() {
    try {
      var buf = self._handle._getPingReq();
      self._write(buf);
    } catch (err) {
      err.message = 'Keepalive Write Error:' + err.message;
      self._closeConnection(err);
      return;
    }
    self._keepAliveTimeout = setTimeout(function() {
      self._closeConnection(new Error('keepalive timeout'));
    }, self._options.pingReqTimeout);
  }, self._options.keepalive * 1000);
};

MqttClient.prototype._onKeepAlive = function() {
  clearTimeout(this._keepAliveTimeout);
  this._keepAlive();
};

MqttClient.prototype._clearKeepAlive = function() {
  clearTimeout(this._keepAliveTimer);
  clearTimeout(this._keepAliveTimeout);
  this._keepAliveTimer = null;
  this._keepAliveTimeout = null;
};

MqttClient.prototype._closeConnection = function(err) {
  if (this._reconnectTimer) {
    clearTimeout(this._reconnectTimer);
    this._reconnectTimer = null;
  }
  if (this._connectTimer) {
    clearTimeout(this._connectTimer);
    this._connectTimer = null;
  }
  if (err) {
    this.emit('error', err);
  }
  if (!this._isConnected) {
    if (this._socket) {
      // force close the socket even if not connected to avoid leak
      this._socket.destroy();
      this._socket = null;
    }
    return;
  }
  try {
    var buf = this._handle._getDisconnect();
    this._socket.end(buf);
  } catch (err) {
    this.emit('error', err);
    this._socket.destroy();
  }
  this._socket = null;
};

MqttClient.prototype.end = function(force, callback) {
  // currently close it right way, ignore in-flight messages ack
  if (typeof force === 'function') {
    callback = force;
  }
  this.disconnect(callback);
};

MqttClient.prototype.disconnect = function(err, callback) {
  this._disconnected = true;
  if (typeof err === 'function') {
    callback = err;
    err = undefined;
  }
  this._closeConnection(err);
  if (callback !== undefined) {
    if (typeof callback !== 'function') {
      throw new Error('callback is not a function');
    }
    this._disconnectCb = callback;
  }
};

MqttClient.prototype._getQoS = function(qos) {
  return qos >= 0 && qos <= 2 ? qos : 0;
};

MqttClient.prototype._getNewPacketId = function() {
  if (this._packetId > MAX_PACKET_ID) {
    this._packetId = 1;
  }

  return this._packetId++;
};

/**
 * @method publish
 * @param {String} topic
 * @param {String} payload
 * @param {Object} options
 * @param {Function} callback
 */
MqttClient.prototype.publish = function(topic, payload, options, callback) {
  callback = callback || noop;

  if (!Buffer.isBuffer(payload)) {
    payload = new Buffer(payload);
  }
  var qos = this._getQoS(options && options.qos);
  try {
    var buf = this._handle._getPublish(topic, {
      id: qos === 0 ? 0 : this._getNewPacketId(),
      qos: qos,
      dup: (options && options.dup) || false,
      retain: (options && options.retain) || false,
      payload: payload,
    });
    this._write(buf, callback);
  } catch (err) {
    callback(err);
  }
};

/**
 * @method subscribe
 * @param {String} topic
 * @param {Object} options
 * @param {Function} callback
 */
MqttClient.prototype.subscribe = function(topic, options, callback) {
  if (!Array.isArray(topic))
    topic = [topic];
  if (typeof options === 'function') {
    callback = options;
    options = { qos: 0 };
  } else {
    callback = callback || noop;
  }
  try {
    var qos = this._getQoS(options && options.qos);
    var buf = this._handle._getSubscribe(topic, {
      id: this._getNewPacketId(),
      qos: qos,
    });
    this._write(buf, callback);
  } catch (err) {
    callback(err);
  }
};

/**
 * @method unsubscribe
 * @param {String} topic
 * @param {Function} callback
 */
MqttClient.prototype.unsubscribe = function(topic, callback) {
  callback = callback || noop;
  if (!Array.isArray(topic)) {
    topic = [topic];
  }
  var buf;
  // TODO don't use try catch
  try {
    buf = this._handle._getUnsubscribe(topic, {
      id: this._getNewPacketId(),
    });
  } catch (err) {
    callback(err);
    return;
  }
  this._write(buf, callback);
};

/**
 * @method reconnect
 */
MqttClient.prototype.reconnect = function() {
  this._closeConnection(null);
  this.connect();
  this.emit('reconnect');
};

function _getLastPacketId() {
  return this._packetId;
}

/**
 * @method getLastPacketId
 */
MqttClient.prototype.getLastPacketId = _getLastPacketId;

/**
 * @deprecated
 * @method getLastMessageId
 */
MqttClient.prototype.getLastMessageId =
util.deprecate(_getLastPacketId, 'getLastMessageId ' +
               'is deprecated, please use `getLastPacketId` instead.');

function connect(endpoint, options) {
  var client = new MqttClient(endpoint, options);
  return client.connect();
}

exports.connect = connect;
