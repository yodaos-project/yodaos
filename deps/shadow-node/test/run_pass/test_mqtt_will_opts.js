'use strict';

var mqtt = require('mqtt');
var assert = require('assert');
var common = require('../common');
var bridge = 'mqtt://localhost:9080';
var willOpts = {
  topic: 'test',
  payload: 'save me',
  qos: 0,
  retain: false,
};

function connect(endpoint, opts) {
  return new Promise(function(resolve, reject) {
    var client = mqtt.connect(endpoint, opts);
    client.once('connect', function() {
      resolve(client);
    });
  });
}

Promise.all([
  connect(bridge, {
    reconnectPeriod: -1,
    will: Object.assign({}, willOpts),
  }),
  connect(bridge, {
    reconnectPeriod: -1,
  }),
]).then((res) => {
  var yorkie = res[0];
  var babeee = res[1];
  babeee.subscribe(willOpts.topic, common.mustCall(function() {
    yorkie._socket.end();
  }));
  babeee.on('message', common.mustCall(function(channel, message) {
    assert.strictEqual(channel, willOpts.topic);
    assert.strictEqual(message + '', willOpts.payload);
    babeee.disconnect();
  }));
});
