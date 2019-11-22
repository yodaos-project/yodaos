'use strict';
var common = require('../../common');
var assert = require('assert');
var child_process = require('child_process');
var test_fatal = require(`./build/Release/test_fatal.node`);

// Test in a child process because the test code will trigger a fatal error
// that crashes the process.
if (process.argv[2] === 'child') {
  test_fatal.Test();
  return;
}

var p = child_process.spawnSync(
  process.execPath, [ __filename, 'child' ]);
assert.ifError(p.error);
assert.ok(p.stderr.toString().includes(
  'FATAL ERROR: test_fatal::Test fatal message'));
