'use strict';
var assert = require('assert');
var common = require('../common.js');

var bench = common.createBenchmark(main, {
  size: [8 << 12],
  n: [32],
});

function repeat(str, size) {
  var r = '';
  for (var i = 0; i < size; i++) {
    r += str;
  }
  return r;
}

function main(opts) {
  var size = opts.size;
  var n = opts.n;
  console.log(size, n);
  var s = repeat('abcd', size);
  // eslint-disable-next-line node-core/no-unescaped-regexp-dot
  s.match(/./);  // Flatten string.
  assert.strictEqual(s.length % 4, 0);
  var b = Buffer.allocUnsafe(s.length / 4 * 3);
  b.write(s, 0, s.length, 'base64');
  bench.start();
  for (var i = 0; i < n; i += 1)
    b.write(s, 0, s.length, 'base64');
  bench.end(n);
}
