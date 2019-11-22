'use strict';
var mqtt = require('mqtt');
var assert = require('assert');
var timeoutHost = 'whatever://google.com:81';
var url = require('url');
var mqttHost = 'mqtts://localhost:9088';
var common = require('../common');

// reconnect after timeout

var client = mqtt.connect(timeoutHost, {
  connectTimeout: 1000,
  reconnectPeriod: 1000
});
client.on('error', common.mustCall(function(err) {
  assert.strictEqual(err.message, 'connect timeout');
}));

client.on('connect', common.mustCall(function() {
  console.log(`connection ${mqttHost} connected`);
  assert.strictEqual(!!client._connectTimer, false);
  assert.strictEqual(!!client._keepAliveTimer, true);
  assert.strictEqual(client._isSocketConnected, true);
  assert.strictEqual(client._isConnected, true);
  assert.strictEqual(!!client._reconnectTimer, false);
  assert.strictEqual(client._disconnected, false);
  client.disconnect();
}));

client.on('close', common.mustCall(function() {
  console.log(`connection ${mqttHost} closed`);
  var obj = url.parse(mqttHost);
  client._host = obj.hostname;
  client._port = Number(obj.port) || 8883;
  client._protocol = obj.protocol;
}));
