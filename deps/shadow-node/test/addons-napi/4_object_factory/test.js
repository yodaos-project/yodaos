'use strict';
var common = require('../../common');
var assert = require('assert');
var addon = require(`./build/Release/binding.node`);

var obj1 = addon('hello');
var obj2 = addon('world');
assert.strictEqual(`${obj1.msg} ${obj2.msg}`, 'hello world');
