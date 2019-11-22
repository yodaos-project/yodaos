'use strict';

var common = require('../../common');
var storeEnv = require(`./build/Release/store_env.node`);
var compareEnv = require(`./build/Release/compare_env.node`);
var assert = require('assert');

assert.strictEqual(compareEnv(storeEnv), true,
                   'N-API environment pointers in two different modules have ' +
                   'the same value');
