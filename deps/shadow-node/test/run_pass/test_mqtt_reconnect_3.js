'use strict';
var mqtt = require('mqtt');
var assert = require('assert');
var mqttHost = 'mqtts://localhost:9088';
var common = require('../common');

// reconnect manually after disconnect

var client = mqtt.connect(mqttHost, {
  connectTimeout: 1000,
  reconnectPeriod: -1
});

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

client.once('close', common.mustCall(function() {
  console.log(`connection ${mqttHost} closed`);
  client.reconnect();
}));
