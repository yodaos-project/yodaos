'use strict';
var test = require('./build/Release/napi_thread_safe.node');

test.sayHelloFromOtherThread(() => {
  process.exit(0);
});
