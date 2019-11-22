'use strict';

var common = require('../../common');
var addon = require(`./build/Release/test_general.node`);
var assert = require('assert');

addon.createNapiError();
assert(addon.testNapiErrorCleanup(), 'napi_status cleaned up for second call');
