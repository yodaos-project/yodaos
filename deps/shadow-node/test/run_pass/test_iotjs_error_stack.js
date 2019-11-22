'use strict';

var assert = require('assert');

var error = getError();
assert(typeof error.stack === 'string');
assert(error.stack.match(/at getError/));
assert(error.stack.match(/\n\n/g) == null);

var plain = getPlainObject();
assert(typeof plain.stack === 'string');
assert(plain.stack.match(/at getPlainObject/));
assert(plain.stack.match(/\n\n/g) == null);

function getError() {
  return new Error('foobar');
}

function getPlainObject() {
  var e = {};
  Error.captureStackTrace(e);
  return e;
}
