'use strict';
var mqtt = require('mqtt');
var assert = require('assert');
var timeoutHost = 'whatever://google.com:81';
var common = require('../common');
var RECONNECT_MAX_TIMES = 5;
var reconnectTimes = 0;

// reconnect 5

var timeoutClient = mqtt.connect(timeoutHost, {
  connectTimeout: 1000,
  reconnectPeriod: 1000
});
timeoutClient.once('connect', function() {
  assert(0, 'should not call this callback');
});
timeoutClient.on('error', common.mustCall(function(err) {
  assert.strictEqual(err.message, 'connect timeout');
}));
timeoutClient.on('close', common.mustCall(function() {
  assert.strictEqual(timeoutClient._connectTimer, null);
  assert.strictEqual(timeoutClient._keepAliveTimer, null);
  assert.strictEqual(timeoutClient._keepAliveTimeout, null);
  assert.strictEqual(timeoutClient._isSocketConnected, false);
  assert.strictEqual(timeoutClient._isConnected, false);
  // reconnect disabled after mqtt.disconnect()
  assert.strictEqual(
    !!timeoutClient._reconnectTimer, reconnectTimes !== RECONNECT_MAX_TIMES);
  assert.strictEqual(
    timeoutClient._disconnected, reconnectTimes === RECONNECT_MAX_TIMES);
}));
timeoutClient.on('reconnect', function() {
  assert(++reconnectTimes <= RECONNECT_MAX_TIMES, 'reconnect to many times');
  console.log(`reconnecting ${timeoutHost}`, reconnectTimes);
  assert.strictEqual(!!timeoutClient._connectTimer, true);
  assert.strictEqual(timeoutClient._keepAliveTimer, null);
  assert.strictEqual(timeoutClient._keepAliveTimeout, null);
  assert.strictEqual(timeoutClient._isSocketConnected, false);
  assert.strictEqual(timeoutClient._isConnected, false);
  assert.strictEqual(!!timeoutClient._reconnectTimer, false);
  assert.strictEqual(timeoutClient._disconnected, false);
  if (reconnectTimes === RECONNECT_MAX_TIMES) {
    timeoutClient.disconnect();
  }
});
