'use strict';

var mqtt = require('mqtt');
var assert = require('assert');
var bridge = 'mqtt://localhost:9080';

var opts = {
  reconnectPeriod: -1,
};

function connect(endpoint, opts) {
  return new Promise(function(resolve, reject) {
    var client = mqtt.connect(endpoint, opts);
    client.once('connect', function() {
      resolve(client);
    });
  });
}

connect(bridge, opts).then((babeee) => {
  babeee.subscribe('u/love7', (err) => {
    assert.strictEqual(babeee._getQoS(0), 0);

    babeee.subscribe('u/love8', {
      qos: 1
    }, (err) => {
      assert.strictEqual(babeee._getQoS(1), 1);

      babeee.subscribe('u/love9', {
        qos: 5
      }, (err) => {
        assert.strictEqual(babeee._getQoS(0), 0);
        babeee.disconnect();
      });
    });
  });
});
