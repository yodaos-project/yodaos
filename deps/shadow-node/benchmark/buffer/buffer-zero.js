'use strict';

var common = require('../common.js');

var bench = common.createBenchmark(main, {
  n: [16],
  type: ['buffer', 'string']
});

var zeroBuffer = Buffer.alloc(0);
var zeroString = '';

function main(opts) {
  var n = opts.n;
  var type = opts.type;
  var data = type === 'buffer' ? zeroBuffer : zeroString;

  bench.start();
  for (var i = 0; i < n * 1024; i++)
    Buffer.from(data);
  bench.end(n);
}
