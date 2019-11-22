'use strict';

var assert = require('assert');

function namedFunction() {
  return process._getStackFrames(11);
}

var frames = namedFunction();
assert(Array.isArray(frames));
assert(frames.length <= 11);

// Expect top frame to be address of namedFunction
assert(frames[0] !== 0);
