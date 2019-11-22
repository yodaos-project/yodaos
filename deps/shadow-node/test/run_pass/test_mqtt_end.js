'use strict';
var mqtt = require('mqtt');
var assert = require('assert');
var mqttHost = 'mqtts://localhost:9088';
var common = require('../common');

function test(cb) {
  var client = mqtt.connect(mqttHost, {
    connectTimeout: 1000,
    reconnectPeriod: -1
  });
  client.once('connect', common.mustCall(function() {
    console.log(`connection ${mqttHost} connected`);
    client.end(cb);
  }));

  client.once('offline', common.mustCall(function() {
    console.log(`connection ${mqttHost} offline`);
  }));

  client.once('close', common.mustCall(function() {
    console.log(`connection ${mqttHost} closed`);
    assert.strictEqual(client._connectTimer, null);
    assert.strictEqual(client._keepAliveTimer, null);
    assert.strictEqual(client._keepAliveTimeout, null);
    assert.strictEqual(client._isSocketConnected, false);
    assert.strictEqual(client._isConnected, false);
    assert.strictEqual(client._reconnectTimer, null);
    assert.strictEqual(client._disconnected, true);
  }));
}

// without cb
test();
// with cb
test(common.mustCall(function() {
  console.log(`connection ${mqttHost} ended`);
}));
