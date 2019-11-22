'use strict';

var common = require('../common.js');
var bench = common.createBenchmark(main, {
  n: [128],
});

function main(opts) {
  var n = opts.n;
  var future = Promise.resolve([]);
  bench.start();

  for (var i = 0; i < n; ++i) {
    future = future.then(() => {
      return Promise.resolve([]);
    });
  }
  future.then(() => {
    bench.end(n);
  });
}
