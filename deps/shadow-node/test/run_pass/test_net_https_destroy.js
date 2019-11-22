'use strict';
var assert = require('assert');
var https = require('https');
var common = require('../common');

var options = {
  hostname: 'example.com',
  port: 443,
  path: '/',
  method: 'GET'
};

var req = https.request(options, (res) => {
  res.on('end', common.mustCall(() => {
    console.log('req end with status code: ', res.statusCode);
    process.nextTick(() => {
      var destroyed = req.socket._socketState.destroyed;
      console.log('req socket is destroyed:', destroyed);
      assert.strictEqual(destroyed, true);
    });
  }));
});
req.end();
