'use strict';
var mqtt = require('mqtt');
var assert = require('assert');
var refusedHost = 'mqtts://localhost:1';
var common = require('../common');
var errCount = 0;
var closeCount = 0;

// test connection refused

var client = mqtt.connect(refusedHost, {
  connectTimeout: 1000,
  reconnectPeriod: -1
});
client.on('error', common.mustCall(function(err) {
  assert.strictEqual(++errCount, 1);
  console.log(`connection ${refusedHost} error: ${err.message}`);
}));
client.on('close', common.mustCall(function() {
  assert.strictEqual(++closeCount, 1);
  console.log(`connection ${refusedHost} closed`);
  assert.strictEqual(client._connectTimer, null);
  assert.strictEqual(client._keepAliveTimer, null);
  assert.strictEqual(client._keepAliveTimeout, null);
  assert.strictEqual(client._isSocketConnected, false);
  assert.strictEqual(client._isConnected, false);
  assert.strictEqual(client._reconnectTimer, null);
  assert.strictEqual(client._disconnected, false);
}));
