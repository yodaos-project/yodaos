'use strict';

var assert = require('assert');
var mod1 = require('./dump/module1');

try {
  mod1();
} catch (err) {
  var stack = err.stack;
  console.log(stack);
  assert.strictEqual(stack.split('\n').length, 4);
}

var mod2 = require('./dump/module2');

function testRepeatThrow() {
  try {
    mod2();
  } catch (err) {
    var stack = err.stack;
    assert.strictEqual(stack.split('\n').length, 5);
  }
}

for (var i = 0; i < 10; i++) {
  testRepeatThrow();
}
console.log('test is done!');
