'use strict';

var common = require('../common.js');
var path = require('path');
var bench = common.createBenchmark(main, {
  n: [512],
});

function main(opts) {
  var n = opts.n;
  var filename = path.join(__dirname, '../common.js');

  bench.start();
  for (var i = 0; i < n; ++i) {
    process.compile(filename);
  }
  bench.end(n);
}
