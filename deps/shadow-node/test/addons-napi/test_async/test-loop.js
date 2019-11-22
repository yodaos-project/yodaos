'use strict';
var common = require('../../common');
var assert = require('assert');
var test_async = require(`./build/Release/test_async.node`);
var iterations = 500;

var x = 0;
var workDone = common.mustCall((status) => {
  assert.strictEqual(status, 0);
  if (++x < iterations) {
    setImmediate(() => test_async.DoRepeatedWork(workDone));
  }
}, iterations);
test_async.DoRepeatedWork(workDone);
