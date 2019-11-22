'use strict';

var common = require('../common.js');
var bench = common.createBenchmark(main, {
  n: [128],
});

function main(opts) {
  var n = opts.n;
  bench.start();
  for (var i = 0; i < n; i += 1)
    console.log('');
  bench.end(n);
}
