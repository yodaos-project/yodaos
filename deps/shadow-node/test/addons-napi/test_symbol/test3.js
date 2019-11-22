'use strict';
var common = require('../../common');
var assert = require('assert');

// testing api calls for symbol
var test_symbol = require(`./build/Release/test_symbol.node`);

assert.notStrictEqual(test_symbol.New(), test_symbol.New());
assert.notStrictEqual(test_symbol.New('foo'), test_symbol.New('foo'));
assert.notStrictEqual(test_symbol.New('foo'), test_symbol.New('bar'));

var foo1 = test_symbol.New('foo');
var foo2 = test_symbol.New('foo');
var object = {
  [foo1]: 1,
  [foo2]: 2,
};
assert.strictEqual(object[foo1], 1);
assert.strictEqual(object[foo2], 2);
