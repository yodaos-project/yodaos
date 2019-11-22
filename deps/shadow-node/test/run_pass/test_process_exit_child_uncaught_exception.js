'use strict';
var child_process = require('child_process');
var assert = require('assert');
var common = require('../common');

if (process.env.CHILD) {
  throw new Error('uncaught exception');
} else {
  var cp = child_process.fork(__filename, [], {
    env: {
      CHILD: 1
    },
    stdio: 'ignore'
  });
  cp.on('exit', common.mustCall(function(code, signal) {
    assert.strictEqual(1, code);
    assert.strictEqual(signal, null);
  }));
}
