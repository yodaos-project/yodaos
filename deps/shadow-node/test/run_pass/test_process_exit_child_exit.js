'use strict';
var child_process = require('child_process');
var assert = require('assert');
var common = require('../common');

var exitCode = 233;

if (process.env.CHILD) {
  process.exit(exitCode);
} else {
  var cp = child_process.fork(__filename, [], {
    env: {
      CHILD: 1
    }
  });
  cp.on('exit', common.mustCall(function(code, signal) {
    assert.strictEqual(exitCode, code);
    assert.strictEqual(signal, null);
  }));
}
