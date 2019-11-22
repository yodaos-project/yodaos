'use strict';
var common = require('../../common');
var assert = require('assert');
var addon = require(`./build/Release/binding.node`);

var obj = new addon.MyObject(9);
assert.strictEqual(obj.value, 9);
obj.value = 10;
assert.strictEqual(obj.value, 10);
assert.strictEqual(obj.plusOne(), 11);
assert.strictEqual(obj.plusOne(), 12);
assert.strictEqual(obj.plusOne(), 13);

assert.strictEqual(obj.multiply().value, 13);
assert.strictEqual(obj.multiply(10).value, 130);

var newobj = obj.multiply(-1);
assert.strictEqual(newobj.value, -13);
assert.notStrictEqual(obj, newobj);
