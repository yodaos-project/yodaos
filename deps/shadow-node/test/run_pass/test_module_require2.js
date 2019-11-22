'use strict';

var assert = require('assert');
var foobar = require('foobar');
assert.strictEqual(foobar, 'foobar');

var test = require('foobar/test');
assert.strictEqual(test, 'test');
