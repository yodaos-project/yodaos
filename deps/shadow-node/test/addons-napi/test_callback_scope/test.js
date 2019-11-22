'use strict';

var common = require('../../common');
var assert = require('assert');
var { runInCallbackScope } = require(`./build/Release/binding.node`);

assert.strictEqual(runInCallbackScope({}, 0, 0, () => 42), 42);

{
  process.once('uncaughtException', common.mustCall((err) => {
    assert.strictEqual(err.message, 'foo');
  }));

  runInCallbackScope({}, 0, 0, () => {
    throw new Error('foo');
  });
}
