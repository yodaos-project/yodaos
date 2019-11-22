'use strict';
var child_process = require('child_process');
var assert = require('assert');
var common = require('../common');

if (process.env.CHILD) {
  process.on('uncaughtException', common.mustCall(function(err) {

  }));
  throw new Error('throw an exception');
} else {
  var cp = child_process.fork(__filename, [], {
    env: {
      CHILD: 1
    },
    stdio: 'ignore'
  });
  cp.on('exit', common.mustCall(function(code, signal) {
    assert.strictEqual(0, code);
    assert.strictEqual(signal, null);
  }));
}
