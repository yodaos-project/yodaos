'use strict';
var assert = require('assert');
var test = require('./build/Release/napi_construct.node');

var val = test.Constructor(123);
assert.strictEqual(val.value, 123);
