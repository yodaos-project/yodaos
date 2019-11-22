'use strict';
var mqtt = require('mqtt');
var assert = require('assert');
var mqttHost = 'mqtt://localhost:9080';
var mqttsHost = 'mqtts://localhost:9088';
var common = require('../common');

function test(testHost) {
  var client = mqtt.connect(testHost, {
    reconnectPeriod: -1
  });
  assert.strictEqual(client.reconnecting, false);
  assert.strictEqual(client._options.keepalive, 60); // default is 60 in seconds
  client.once('connect', function() {
    assert.strictEqual(client.reconnecting, false);
    assert.strictEqual(client.connected, true);
    assert.strictEqual(client._isSocketConnected, true);
    // trigger the close event
    client.once('offline', common.mustCall(function() {
      assert.strictEqual(client.reconnecting, false);
      assert.strictEqual(client.connected, false);
      assert.strictEqual(client._isSocketConnected, false);
    }));
    client.disconnect();
    client.once('close', common.mustCall(function() {
      assert.strictEqual(client._keepAliveTimer, null);
      assert.strictEqual(client._keepAliveTimeout, null);
    }));
  });
  // the close event will be triggered whether testHost is connected or not
  client.once('close', common.mustCall(function() {
    assert.strictEqual(client.reconnecting, false);
    assert.strictEqual(client.connected, false);
    assert.strictEqual(client._isSocketConnected, false);
  }));
}

test(mqttHost);
test(mqttsHost);
