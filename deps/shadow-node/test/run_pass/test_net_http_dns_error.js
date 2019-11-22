'use strict';

var assert = require('assert');
var http = require('http');
var common = require('../common');

var host = 'www.unable-to-resolved-host.rokid.com';
var req = http.request({ host: host });

req.socket.on('finish', common.mustCall(() => {
  process.nextTick(() => {
    var destroyed = req.socket._socketState.destroyed;
    console.log(`socket is destroyed ${host}:`, destroyed);
    assert.strictEqual(destroyed, true);
  });
}));

// handle error event to prevent the process throw an error
req.on('error', common.mustCall((err) => {
  console.log('request error', err.message);
}));
