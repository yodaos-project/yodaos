'use strict';
var mqtt = require('mqtt');
var assert = require('assert');
var timeoutHost = 'whatever://google.com:81';
var common = require('../common');

var reconnectTimes = 0;
var RECONNECT_MAX_TIMES = 5;
var client = mqtt.connect(timeoutHost, {
  connectTimeout: 500,
  reconnectPeriod: 100
});

// 1. reconnect 5 times after connect timeout
// 2. disconnect

var events = [];
var EVENTS_ORDER = [
  'error', 'close', 'reconnect',
  'error', 'close', 'reconnect',
  'error', 'close', 'reconnect',
  'error', 'close', 'reconnect',
  'error', 'close', 'reconnect',
  'close', 'disconnect'
];

function checkEvents() {
  assert.strictEqual(events.length, EVENTS_ORDER.length);
  for (var i = 0; i < events.length; ++i) {
    assert.strictEqual(events[i], EVENTS_ORDER[i]);
  }
}

client.on('close', common.mustCall(function() {
  console.log(`connection ${timeoutHost} closed`);
  events.push('close');
}));

client.on('error', common.mustCall(function(err) {
  console.log(`connection ${timeoutHost} error: ${err.message}`);
  events.push('error');
}));

client.on('reconnect', common.mustCall(function() {
  console.log(`connection ${timeoutHost} reconnect`);
  events.push('reconnect');
  if (++reconnectTimes === RECONNECT_MAX_TIMES) {
    client.disconnect(common.mustCall(function() {
      events.push('disconnect');
      checkEvents();
    }));
  }
}));
