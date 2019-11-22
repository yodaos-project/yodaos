'use strict';

var common = require('../common.js');
var bench = common.createBenchmark(main, {
  n: [1024],
});

function main(opts) {
  var n = opts.n;
  process.nextTick(function() {
    bench.start();
    for (var i = 0; i < n; ++i) {
      process.nextTick(function() { }, 1, 2, 3, 4, 5);
    }
    process.nextTick(function() {
      bench.end(n);
    });
  });
}
