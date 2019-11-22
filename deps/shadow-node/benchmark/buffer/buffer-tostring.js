'use strict';

var common = require('../common.js');
var bench = common.createBenchmark(main, {
  encoding: ['', 'utf8'],
  args: [0, 1, 2, 3],
  len: [0, 1, 64],
  n: [1024]
});

function main(opts) {
  var encoding = opts.encoding;
  var args = opts.args;
  var len = opts.len;
  var n = opts.n;
  var buf = Buffer.alloc(len, 42);

  if (encoding.length === 0)
    encoding = undefined;

  var i;
  switch (args) {
    case 1:
      bench.start();
      for (i = 0; i < n; i += 1)
        buf.toString(encoding);
      bench.end(n);
      break;
    case 2:
      bench.start();
      for (i = 0; i < n; i += 1)
        buf.toString(encoding, 0);
      bench.end(n);
      break;
    case 3:
      bench.start();
      for (i = 0; i < n; i += 1)
        buf.toString(encoding, 0, len);
      bench.end(n);
      break;
    default:
      bench.start();
      for (i = 0; i < n; i += 1)
        buf.toString();
      bench.end(n);
      break;
  }
}
