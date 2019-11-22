'use strict';

var assert = require('assert');
assert.strictEqual(/(\/)?iotjs$/.test(process.title), true);

process.title = 'foobar';
assert.strictEqual(process.title, 'foobar');
