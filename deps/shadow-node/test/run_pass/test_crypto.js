'use strict';
var assert = require('assert');
var crypto = require('crypto');

var r1 = crypto.randomBytes(10);
assert.strictEqual(r1 instanceof Buffer, true);
assert.strictEqual(r1.length, 10);
assert.strictEqual(r1.toString('base64').length, 16);
assert.strictEqual(r1.toString('hex').length, 20);

(function randomTest(len) {
  var map = {};
  var curr;
  for (var i = 0; i < len; i++) {
    curr = crypto.randomBytes(10).toString('hex');
    if (map[curr])
      assert.fail();
    else
      map[curr] = 1;
  }
})(1000);
