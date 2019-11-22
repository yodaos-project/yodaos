'use strict';
var common = require('../../common');
var assert = require('assert');

var test_globals = require(`./build/Release/test_general.node`);

assert.strictEqual(test_globals.getUndefined(), undefined);
assert.strictEqual(test_globals.getNull(), null);
