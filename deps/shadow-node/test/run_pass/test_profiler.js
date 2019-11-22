'use strict';

/* This test need custom build
 * $ShadowNode ./tools/build.py --jerry-heap-profiler --jerry-cpu-profiler
 */
var profiler = require('profiler');

/* eslint-disable-next-line no-unused-vars */
var obj = {
  foo: 'bar',
  [Symbol('foo')]: 'bar',
};
profiler.takeSnapshot('test.heapdump');

profiler.startProfiling('test.cpuprof');
console.log('cpu profiling starts');

function test() {
  Array.isArray([]);
  Array.isArray([]);
  Object.keys({});
}
for (var i = 0; i < 10; i++) {
  test();
}

setTimeout(function foobar() {
  var profile = profiler.stopProfiling();
  console.log(profile);
}, 1000);
