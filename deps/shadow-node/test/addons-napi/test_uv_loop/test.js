'use strict';
var common = require('../../common');
var { SetImmediate } = require(`./build/Release/test_uv_loop.node`);

SetImmediate(common.mustCall());
