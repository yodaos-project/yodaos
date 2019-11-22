'use strict';

var assert = require('assert');
var constants = require('constants');
var os = require('os');

function testPriority(val) {
  assert.strictEqual(os.setPriority(val), undefined);
  assert.strictEqual(os.getPriority(), val);
}

function testPriorityOutOfRange(val) {
  assert.throws(
    function() {
      os.setPriority(val);
    },
    RangeError
  );
}

testPriority(1);
testPriority(2);
testPriority(3);

testPriorityOutOfRange(constants.UV_PRIORITY_LOW + 1);
testPriorityOutOfRange(constants.UV_PRIORITY_HIGHEST - 1);

// test without access
var thrown = false;
try {
  os.setPriority(-10);
} catch (err) {
  thrown = true;
}

var priority = os.getPriority();
console.log('set priority', priority);

if (thrown) {
  assert.strictEqual(priority, 3);
} else {
  assert.strictEqual(priority, -10);
}
