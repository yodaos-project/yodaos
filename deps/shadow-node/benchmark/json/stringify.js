'use strict';

var common = require('../common.js');
var bench = common.createBenchmark(main, {
  n: [128],
});

var obj = {};
for (var i = 0; i < 1000; i++) {
  obj['foo' + i] = i * 10;
}

function main(opts) {
  var n = opts.n;
  bench.start();
  for (var i = 0; i < n; ++i)
    JSON.stringify(obj);
  bench.end(n);
}
