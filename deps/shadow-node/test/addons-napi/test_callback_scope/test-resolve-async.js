'use strict';

var common = require('../../common');
var { testResolveAsync } = require(`./build/Release/binding.node`);

testResolveAsync().then(common.mustCall());
