'use strict';

var common = require('../common.js');
var bench = common.createBenchmark(main, {
  n: [128],
});

function test2() {
  throw new Error('yo');
}

function test() {
  test2();
}

function main(opts) {
  var n = opts.n;
  bench.start();
  for (var i = 0; i < n; i += 1) {
    try {
      test();
    } catch (err) {
      // eslint-disable-next-line no-unused-vars
      var stack = err.stack;
    }
  }
  bench.end(n);
}
