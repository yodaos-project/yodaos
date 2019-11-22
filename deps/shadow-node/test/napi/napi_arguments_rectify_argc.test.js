'use strict';
var assert = require('assert');
var test = require('./build/Release/napi_arguments.node');

assert.strictEqual(test.RectifyArgc('foo', 'bar'), 2);
assert.strictEqual(test.RectifyArgc(), 0);

var argv = [];
for (var i = 0; i < 50; i++) {
  argv.push(i);
}
assert.strictEqual(test.RectifyArgc.apply(test, argv), argv.length);
