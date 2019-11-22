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
  babeee.subscribe('u/love4', (err) => {
    assert.strictEqual(babeee.getLastPacketId(), 2);
    babeee.subscribe('u/love5', (err) => {
      assert.strictEqual(babeee.getLastPacketId(), 3);
      babeee.subscribe('u/love6', (err) => {
        assert.strictEqual(babeee.getLastPacketId(), 4);
        babeee.disconnect();
      });
    });
  });
});
