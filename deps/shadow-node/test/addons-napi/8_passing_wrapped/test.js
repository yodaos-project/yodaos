'use strict';
// Flags: --expose-gc

var common = require('../../common');
var assert = require('assert');
var addon = require(`./build/Release/binding.node`);

var obj1 = addon.createObject(10);
var obj2 = addon.createObject(20);
var result = addon.add(obj1, obj2);
assert.strictEqual(result, 30);

// Make sure the native destructor gets called.
obj1 = null;
obj2 = null;
global.gc();
assert.strictEqual(addon.finalizeCount(), 2);
