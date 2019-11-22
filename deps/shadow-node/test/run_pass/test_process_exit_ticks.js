'use strict';
var assert = require('assert');

setTimeout(function() {
  assert.fail();
}, 0);

setImmediate(function() {
  assert.fail();
});

setInterval(function() {
  assert.fail();
}, 0);

process.nextTick(function() {
  assert.fail();
});

process.exit(0);
