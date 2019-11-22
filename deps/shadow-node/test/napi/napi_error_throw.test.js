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
  test.ThrowError();
  assert.fail('fail path');
} catch (err) {
  assert(err != null);
  assert.strictEqual(err.message, 'foobar');
}

try {
  test.ThrowCreatedError();
  assert.fail('fail path');
} catch (err) {
  assert(err != null);
  assert.strictEqual(err.message, 'foobar');
}

var err = test.GetError();
assert(err != null);
assert.strictEqual(err.message, 'foobar');
assert.strictEqual(err.code, '');

err = test.GetNoCodeError();
assert(err != null);
assert.strictEqual(err.message, 'foobar');
assert.strictEqual(err.code, undefined);
