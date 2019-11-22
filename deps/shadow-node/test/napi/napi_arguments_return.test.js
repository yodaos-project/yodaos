'use strict';
var assert = require('assert');
var test = require('./build/Release/napi_arguments.node');


var obj = {};
assert.strictEqual(test.Return(obj), obj);
