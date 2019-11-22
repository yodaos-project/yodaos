'use strict';
var util = require('util');
var assert = require('assert');

var ret;

function resource(throws, argv, callback) {
  setTimeout(() => {
    if (throws) {
      callback(argv);
      return;
    }
    callback(null, argv);
  }, 10);
}

var resourceAsync = util.promisify(resource);
ret = resourceAsync(false, 'foobar')
  .then((it) => {
    assert.strictEqual(it, 'foobar');
  });

assert(ret instanceof Promise);

ret = resourceAsync(true, new Error('foobar'))
  .then(
    () => assert.fail('unreachable path'),
    (err) => {
      assert(err != null);
      assert.strictEqual(err.message, 'foobar');
    });

assert(ret instanceof Promise);

function resourceCustom() {
  assert.fail('unreachable path');
}
resourceCustom[util.promisify.custom] = function resourceCustomAsync(argv) {
  return new Promise((resolve) => resolve(argv));
};
var resourceCustomAsync = util.promisify(resourceCustom);
assert.strictEqual(resourceCustomAsync, resourceCustom[util.promisify.custom]);
ret = resourceCustomAsync('foobar')
  .then((it) => assert.strictEqual(it, 'foobar'));
assert(ret instanceof Promise);
