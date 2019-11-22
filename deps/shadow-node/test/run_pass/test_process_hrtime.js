'use strict';

var assert = require('assert');

var NANOSECOND_PER_SECONDS = 1e9;

var time = process.hrtime();
assert(Array.isArray(time));
assert(time.length === 2);

var diff = process.hrtime(time);
assert(Array.isArray(diff));
assert(diff.length === 2);
diff = diff[0] * NANOSECOND_PER_SECONDS + diff[1];

var curr = process.hrtime();
var diff2 = (curr[0] * NANOSECOND_PER_SECONDS + curr[1]) -
  (time[0] * NANOSECOND_PER_SECONDS + time[1]);
assert(diff2 - diff > 0);

assert.throws(function() {
  process.hrtime('foobar');
}, TypeError);

assert.throws(function() {
  process.hrtime([1 ]);
}, TypeError);

assert.throws(function() {
  process.hrtime([1, 2, 3]);
}, TypeError);
