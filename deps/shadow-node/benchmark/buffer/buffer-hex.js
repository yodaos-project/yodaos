'use strict';

var common = require('../common.js');
var bench = common.createBenchmark(main, {
  len: [64],
  n: [4 * 1024]
});

function main(opts) {
  var len = opts.len;
  var n = opts.n;
  var buf = Buffer.alloc(len);
  var i;

  for (i = 0; i < buf.length; i++)
    buf[i] = i & 0xff;

  var hex = buf.toString('hex');
  bench.start();

  for (i = 0; i < n; i += 1)
    Buffer.from(hex, 'hex');

  bench.end(n);
}
