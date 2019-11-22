'use strict';
var http = require('http');
var https = require('https');
var assert = require('assert');
var common = require('../common');

function getHandle(url) {
  if (/^http:\/\//.test(url)) {
    return http;
  } else if (/^https:\/\//.test(url)) {
    return https;
  }
  throw new Error('unsupported url ' + url);
}

// test http_client
test('http://example.com/');
// test https_client
test('https://example.com/');
// unreachable tunnels, manually abort them
var req1 = test('http://127.0.0.2');
setTimeout(function() {
  req1.abort();
}, 1000);
var req2 = test('https://127.0.0.2');
setTimeout(function() {
  req2.abort();
}, 1000);

function test(url) {
  var isAborted = false;
  var eventTriggered = false;
  var chunks = [];
  var handle = getHandle(url);
  var req = handle.get(url, function(res) {
    res.on('data', function(chunk) {
      assert.strictEqual(isAborted, false, `${url} should not aborted`);
      isAborted = true;
      req.abort();
      process.nextTick(function() {
        assert.strictEqual(eventTriggered, true,
                           `${url} should trigger event abort`);
      });
      chunks.push(chunk);
    });

  });

  req.on('abort', common.mustCall(function() {
    eventTriggered = true;
  }));

  req.socket.on('close', common.mustCall(function() {
  }));

  req.end();
  return req;
}
