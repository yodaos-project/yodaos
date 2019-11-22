'use strict';
var mqtt = require('mqtt');
var assert = require('assert');
var timeoutHost = 'whatever://google.com:81';
var common = require('../common');

var client = mqtt.connect(timeoutHost, {
  connectTimeout: 1000,
  reconnectPeriod: -1
});

// 1. connect timeout without reconnect

var events = [];
var EVENTS_ORDER = ['error', 'close'];

function checkEvents() {
  assert.strictEqual(events.length, EVENTS_ORDER.length);
  for (var i = 0; i < events.length; ++i) {
    assert.strictEqual(events[i], EVENTS_ORDER[i]);
  }
}

client.on('close', common.mustCall(function() {
  events.push('close');
  checkEvents();
}));

client.on('error', common.mustCall(function() {
  events.push('error');
}));
