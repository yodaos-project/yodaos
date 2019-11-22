'use strict';
var common = require('../../common');
var assert = require('assert');

// testing api calls for symbol
var test_symbol = require(`./build/Release/test_symbol.node`);

var sym = test_symbol.New('test');
assert.strictEqual(sym.toString(), 'Symbol(test)');

var myObj = {};
var fooSym = test_symbol.New('foo');
var otherSym = test_symbol.New('bar');
myObj.foo = 'bar';
myObj[fooSym] = 'baz';
myObj[otherSym] = 'bing';
assert.strictEqual(myObj.foo, 'bar');
assert.strictEqual(myObj[fooSym], 'baz');
assert.strictEqual(myObj[otherSym], 'bing');
