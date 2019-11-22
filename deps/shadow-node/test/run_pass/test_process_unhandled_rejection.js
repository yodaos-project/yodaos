'use strict';
var assert = require('assert');
var future;

process.on('unhandledRejection', (err, promise) => {
  assert.strictEqual(err.message, 'foobar');
  assert(promise instanceof Promise);
  assert.strictEqual(promise, future);
});

future = asyncFunction();
function asyncFunction() {
  return Promise.reject(new Error('foobar'));
}
