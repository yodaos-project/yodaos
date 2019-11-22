'use strict';

var common = require('../common.js');
var EventEmitter = require('events');
var emitter = new EventEmitter();

var bench = common.createBenchmark(main, {
  n: [10]
});

function noop() {}

function main(opts) {
  var n = opts.n;
  emitter.on('newListener', noop);
  bench.start();
  for (var i = 0; i < n; ++i)
    emitter.on('hello', noop);
  bench.end(n);
}
