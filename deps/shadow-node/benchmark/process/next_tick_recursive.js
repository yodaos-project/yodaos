'use strict';

var common = require('../common.js');
var bench = common.createBenchmark(main, {
  n: [1024],
});

function main(opts) {
  var n = opts.n;
  var i = 0;
  function runTasks() {
    if (i === 0) {
      bench.start();
    }
    if (i++ < n) {
      process.nextTick(runTasks);
    } else {
      bench.end(n);
    }
  }

  process.nextTick(runTasks);
}
