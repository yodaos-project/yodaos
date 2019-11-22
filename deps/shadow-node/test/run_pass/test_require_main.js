'use strict';

var assert = require('assert');

assert.strictEqual(require.main === module, true);

var equal_when_require = require('./require4/foo');
assert.strictEqual(equal_when_require(), false);
