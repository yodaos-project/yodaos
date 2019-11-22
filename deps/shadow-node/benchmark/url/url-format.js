'use strict';

var common = require('../common.js');
var bench = common.createBenchmark(main, {
  n: [128],
});

var url = require('url');
var urlObject = require('./url.json');

function main(opts) {
  var n = opts.n;
  bench.start();
  for (var i = 0; i < n; ++i)
    url.format(urlObject);
  bench.end(n);
}
