'use strict';
var common = require('../../common');
var assert = require('assert');
var child_process = require('child_process');
var test_async = require(`./build/Release/test_async.node`);

var testException = 'test_async_cb_exception';

// Exception thrown from async completion callback.
// (Tested in a spawned process because the exception is fatal.)
if (process.argv[2] === 'child') {
  test_async.Test(1, {}, common.mustCall(function() {
    throw new Error(testException);
  }));
  return;
}
var p = child_process.spawnSync(
  process.execPath, [ __filename, 'child' ]);
assert.ifError(p.error);
assert.ok(p.stderr.toString().includes(testException));

// Successful async execution and completion callback.
test_async.Test(5, {}, common.mustCall(function(err, val) {
  assert.strictEqual(err, null);
  assert.strictEqual(val, 10);
  process.nextTick(common.mustCall());
}));

// Async work item cancellation with callback.
test_async.TestCancel(common.mustCall());
