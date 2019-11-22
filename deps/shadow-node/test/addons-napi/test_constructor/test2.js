'use strict';
var common = require('../../common');
var assert = require('assert');

// Testing api calls for a constructor that defines properties
var TestConstructor =
    require(`./build/Release/test_constructor_name.node`);
assert.strictEqual(TestConstructor.name, 'MyObject');
