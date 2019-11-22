'use strict';
var assert = require('assert');
var common = require('../common');

var fn = (c) => {
  assert.strictEqual(0, c);
};

process.on('exit', function(c) {
  fn(c);
  process.exit(0);
});

// fn should be call only once
fn = common.mustCall(fn, 1);
