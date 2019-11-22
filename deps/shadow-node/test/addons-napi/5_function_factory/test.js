'use strict';
var common = require('../../common');
var assert = require('assert');
var addon = require(`./build/Release/binding.node`);

var fn = addon();
assert.strictEqual(fn(), 'hello world'); // 'hello world'
