'use strict';
var common = require('../common.js');

var bench = common.createBenchmark(main, {
  len: [16 * 1024],
  n: [32]
});

function main(opts) {
  var len = opts.len;
  var n = opts.n;
  var b = Buffer.allocUnsafe(len);
  var s = '';
  var i;
  for (i = 0; i < 256; ++i)
    s += String.fromCharCode(i);
  for (i = 0; i < len; i += 256)
    b.write(s, i, 256, 'ascii');

  bench.start();
  for (i = 0; i < n; ++i)
    b.toString('base64');
  bench.end(n);
}
