'use strict';
// Flags: --expose-gc

var common = require('../../common');
var assert = require('assert');
var test = require(`./build/Release/binding.node`);

assert.strictEqual(test.finalizeCount, 0);
(() => {
  var obj = test.createObject(10);
  assert.strictEqual(obj.plusOne(), 11);
  assert.strictEqual(obj.plusOne(), 12);
  assert.strictEqual(obj.plusOne(), 13);
})();
global.gc();
assert.strictEqual(test.finalizeCount, 1);

(() => {
  var obj2 = test.createObject(20);
  assert.strictEqual(obj2.plusOne(), 21);
  assert.strictEqual(obj2.plusOne(), 22);
  assert.strictEqual(obj2.plusOne(), 23);
})();
global.gc();
assert.strictEqual(test.finalizeCount, 2);
