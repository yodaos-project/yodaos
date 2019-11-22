'use strict';
var assert = require('assert');

var test;
try {
  require('./build/Release/napi_error.node');
  assert.fail('unreachable path');
} catch (error) {
  assert.strictEqual(error.message, 'foobar');
  test = error;
}

try {
  test.RethrowError();
  assert.fail('fail path');
} catch (err) {
  assert(err != null);
  assert.strictEqual(err.message, 'foobar');
}
