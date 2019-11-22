'use strict';

var tty = require('tty');
var assert = require('assert');

assert.strictEqual(true, tty.isatty(0));
assert.strictEqual(true, tty.isatty(1));
assert.strictEqual(true, tty.isatty(2));
assert.strictEqual(false, tty.isatty(3));
assert.strictEqual(false, tty.isatty(4));
assert.strictEqual(false, tty.isatty(5));

assert.throws(() => {
  tty.isatty(100000);
}, Error);
