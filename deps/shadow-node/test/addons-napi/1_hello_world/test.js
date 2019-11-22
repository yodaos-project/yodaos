'use strict';
var common = require('../../common');
var assert = require('assert');
var bindingPath = require.resolve(`./build/Release/binding.node`);
var binding = require(bindingPath);
assert.strictEqual(binding.hello(), 'world');
console.log('binding.hello() =', binding.hello());

// Test multiple loading of the same module.
delete require.cache[bindingPath];
var rebinding = require(bindingPath);
assert.strictEqual(rebinding.hello(), 'world');
assert.notStrictEqual(binding.hello, rebinding.hello);
