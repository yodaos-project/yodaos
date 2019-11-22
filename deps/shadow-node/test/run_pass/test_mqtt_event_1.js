'use strict';
var mqtt = require('mqtt');
var assert = require('assert');
var mqttHost = 'mqtts://localhost:9088';
var common = require('../common');

var client = mqtt.connect(mqttHost, {
  connectTimeout: 1000,
  reconnectPeriod: -1
});

// 1.connect
// 2.disconnect

var events = [];
var EVENTS_ORDER = ['connect', 'offline', 'close', 'disconnect'];

function checkEvents() {
  assert.strictEqual(events.length, EVENTS_ORDER.length);
  for (var i = 0; i < events.length; ++i) {
    assert.strictEqual(events[i], EVENTS_ORDER[i]);
  }
}

client.on('connect', common.mustCall(function() {
  events.push('connect');
  client.disconnect(common.mustCall(function() {
    events.push('disconnect');
    checkEvents();
  }));
}));

client.on('offline', common.mustCall(function() {
  events.push('offline');
}));

client.on('close', common.mustCall(function() {
  events.push('close');
}));
