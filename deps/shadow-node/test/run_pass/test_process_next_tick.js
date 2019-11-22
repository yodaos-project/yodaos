/* Copyright 2015-present Samsung Electronics Co., Ltd. and other contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
'use strict';

var assert = require('assert');


var trace1 = '';
var trace2 = '';
var trace3 = '';
var trace4 = '';
var trace5 = '';
var trace6 = '';

function test1() {
  process.nextTick(function() {
    trace1 += '1';
    process.nextTick(function() {
      trace1 += '2';
      process.nextTick(function() {
        trace1 += '3';
        process.nextTick(function() {
          trace1 += '4';
          process.nextTick(function() {
            trace1 += '5';
          });
        });
      });
    });
  });
}

function test2() {
  process.nextTick(function() {
    trace2 += '1';
  });
  process.nextTick(function() {
    trace2 += '2';
  });
  setImmediate(function() {
    trace2 += '3';
    process.nextTick(function() {
      trace2 += '5';
    });
  });
  setImmediate(function() {
    trace2 += '4';
  });
}

function test3() {
  process.nextTick(function() {
    trace3 += '1';
  });
  process.nextTick(function() {
    trace3 += '2';
  });
  setTimeout(function() {
    trace3 += '3';
    // FIXME
    // when the timer is triggered,
    // the js callback is called by iotjs_make_callback and therefore
    // the nextTick callbacks is called
    process.nextTick(function() {
      trace3 += '4';
    });
  }, 0);
  setTimeout(function() {
    trace3 += '5';
  }, 0);
}

function test4() {
  process.nextTick(function() {
    trace4 += '3';
  });

  new Promise(function(resolve) {
    trace4 += '1';
    resolve();
    trace4 += '2';
  }).then(function() {
    trace4 += '4';
  });

  process.nextTick(function() {
    trace4 += '5';
  });
}

function test5() {
  setTimeout(function() {
    trace5 += '5';
  }, 0);

  new Promise(function(resolve, reject) {
    trace5 += '1';
    resolve();
  }).then(function() {
    trace5 += '2';
  }).then(function() {
    trace5 += '4';
  });

  process.nextTick(function() {
    trace5 += '3';
  });
}

function test6() {
  process.nextTick(function() {
    trace6 += arguments[0] + arguments[1] + arguments[2] +
      arguments[3] + arguments[4];
  }, '1', '2', '3', '4', '5');
}

function test7() {
  process.nextTick(function() {
    assert.ok(arguments[0] === undefined);
    assert.ok(arguments[1] === undefined);
  });
}

function test8() {
  process.nextTick(function() {
    assert.ok(arguments[0] === '1');
    assert.ok(arguments[1] === '2');
    assert.ok(arguments[2] === '3');
    assert.ok(arguments[3] === '4');
    assert.ok(arguments[4] === '5');
  }, '1', '2', '3', '4', '5');
}

function test9() {
  var err;
  try {
    process.nextTick();
  } catch (e) {
    err = e;
  } finally {
    assert.ok(err);
  }
}

test1();
test2();
test3();
test4();
test5();
test6();
test7();
test8();
test9();

process.on('exit', function(code) {
  assert.strictEqual(code, 0);
  assert.strictEqual(trace1, '12345');
  assert.strictEqual(trace2, '12345');
  assert.strictEqual(trace3, '12345');
  assert.strictEqual(trace4, '12345');
  assert.strictEqual(trace5, '12345');
  assert.strictEqual(trace6, '12345');
});
