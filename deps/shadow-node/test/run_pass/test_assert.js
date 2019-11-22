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

/* eslint-disable no-restricted-syntax,no-restricted-properties,
                  no-self-compare */
assert.ok(1 === 1);

assert.strictEqual(1, 1);
assert.notStrictEqual(1, 2);

assert.strictEqual(0, 0);
assert.throws(function() {
  assert.strictEqual(1, 0);
}, assert.AssertionError);

assert.deepStrictEqual({
  a: 1
}, {
  a: 1
});
assert.deepStrictEqual([1, 2, 3], [1, 2, 3]);
assert.throws(function() {
  assert.deepStrictEqual({
    a: 1
  }, {
    a: 0
  });
}, assert.AssertionError);

assert.throws(function() {
  assert.deepStrictEqual([1, 2, 3], [1, 2]);
}, assert.AssertionError);

assert.throws(function() {
  assert.notStrictEqual(true, true);
}, assert.AssertionError);

assert.throws(function() {
  assert.notStrictEqual(true, true);
}, assert.AssertionError, 'something');

assert.throws(function() {
  assert.throws(function() {});
}, assert.AssertionError);

assert.throws(function() {
  assert.throws(function() {
    assert.strictEqual(1, 0);
  }, TypeError);
}, assert.AssertionError);

assert.doesNotThrow(function() {
  assert.strictEqual(1, 1);
}, 'something');

assert.throws(function() {
  assert.strictEqual(1, 0);
}, assert.AssertionError, assert.AssertionError.name = 'something');

assert.equal(0, false);
assert.notStrictEqual(0, false);

assert.throws(
  function() {
    assert.strictEqual(1, 2);
  },
  assert.AssertionError
);

assert.throws(
  function() {
    assert.ok(1 == 2);
  },
  assert.AssertionError
);

assert.doesNotThrow(
  function() {
    assert.ok(1 === 1);
  }
);

assert.throws(
  function() {
    assert.doesNotThrow(
      function() {
        assert.ok(1 == 2);
      }
    );
  },
  assert.AssertionError
);

assert.throws(() => {
  throw new Error('foobar');
}, /foobar/);

assert.throws(() => {
  throw new Error('foobar');
}, {
  message: 'foobar',
});

assert.throws(() => {
  throw new Error('foobar');
}, (err) => {
  assert(err instanceof Error);
  assert.strictEqual(err.message, 'foobar');
  return true;
});

try {
  assert.ok(false, 'assert test');
} catch (e) {
  assert.strictEqual(e.name, 'AssertionError');
  assert.strictEqual(e.actual, false);
  assert.strictEqual(e.expected, true);
  assert.strictEqual(e.operator, '==');
  assert.strictEqual(e.message, 'assert test');
}

try {
  assert.equal(1, 2, 'assert.equal test');
} catch (e) {
  assert.strictEqual(e.name, 'AssertionError');
  assert.strictEqual(e.actual, 1);
  assert.strictEqual(e.expected, 2);
  assert.strictEqual(e.operator, '==');
  assert.strictEqual(e.message, 'assert.equal test');
}


try {
  assert.fail('actual', 'expected', 'message', 'operator');
} catch (e) {
  assert.strictEqual(e.name, 'AssertionError');
  assert.strictEqual(e.actual, 'actual');
  assert.strictEqual(e.expected, 'expected');
  assert.strictEqual(e.operator, 'operator');
  assert.strictEqual(e.message, 'message');
}
