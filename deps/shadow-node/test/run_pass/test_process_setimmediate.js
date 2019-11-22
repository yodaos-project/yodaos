'use strict';
var assert = require('assert');
var fs = require('fs');

var expectedResult = [0, 1, 2];
var result = [];

fs.readFile(__filename, () => {
  setTimeout(() => {
    result.push(2);
  }, 0);
  setImmediate(() => {
    result.push(1);
  });
  process.nextTick(() => {
    result.push(0);
  });
});

process.on('exit', () => {
  assert.deepStrictEqual(result, expectedResult);
});
