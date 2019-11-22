'use strict';
var assert = require('assert');
var common = require('../common');

var fn = (c) => {
  assert.strictEqual(0, c);
};

process.on('exit', function(c) {
  fn(c);
});

fn = common.mustCall(fn);
