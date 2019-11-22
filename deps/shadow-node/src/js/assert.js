// Originally from narwhal.js (http://narwhaljs.org)
// Copyright (c) 2009 Thomas Robinson <280north.com>
// Copyright (c) 2015 Samsung Electronics Co., Ltd.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the 'Software'), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

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

var util = require('util');


function AssertionError(options) {
  this.name = 'AssertionError';
  this.actual = options.actual;
  this.expected = options.expected;
  this.operator = options.operator;
  if (options.message) {
    this.message = options.message;
  } else {
    this.message = getMessage(this);
  }
  Error.captureStackTrace(this, AssertionError);
}
util.inherits(AssertionError, Error);


function getMessage(assertion) {
  return JSON.stringify(assertion, ['actual', 'expected', 'operator']);
}


function assert(value, message) {
  if (!value) {
    fail(value, true, message, '==');
  }
}


function fail(actual, expected, message, operator) {
  throw new AssertionError({
    message: message,
    actual: actual,
    expected: expected,
    operator: operator,
  });
}


function equal(actual, expected, message) {
  // eslint-disable-next-line eqeqeq
  if (actual != expected) {
    fail(actual, expected, message, '==');
  }
}


function notEqual(actual, expected, message) {
  // eslint-disable-next-line eqeqeq
  if (actual == expected) {
    fail(actual, expected, message, '!=');
  }
}


function strictEqual(actual, expected, message) {
  if (actual !== expected) {
    fail(actual, expected, message, '===');
  }
}

function strictDeepEqual(val1, val2) {
  if (util.isNumber(val1)) {
    if (util.isNumber(val2)) {
      return (isNaN(val1) && isNaN(val2)) || (val1 === val2);
    } else {
      return false;
    }
  }
  if (Array.isArray(val1)) {
    if (Array.isArray(val2) && val1.length === val2.length) {
      var i;
      for (i = 0; i < val1.length; i++) {
        if (!isDeepStrictEqual(val1[i], val2[i])) {
          return false;
        }
      }
      return true;
    } else {
      return false;
    }
  }
  if (util.isDate(val1) && util.isDate(val2)) {
    return val1.getTime() === val2.getTime();
  }
  if (util.isRegExp(val1) && util.isRegExp(val1)) {
    return val1.source === val2.source && val1.flags === val2.flags;
  }
  if (util.isError(val1) && util.isError(val2)) {
    return val1.message !== val2.message;
  }
  var aKeys = Object.keys(val1);
  if (aKeys.length !== Object.keys(val2).length) {
    return false;
  }
  for (i = 0; i < aKeys.length; i++) {
    var key = aKeys[i];
    if (!val2.hasOwnProperty(key) || val1[key] !== val2[key]) {
      return false;
    }
  }
  return true;
}

function isDeepStrictEqual(val1, val2) {
  if (val1 && val1 === val2) {
    return true;
  }
  return strictDeepEqual(val1, val2);
}


function deepStrictEqual(actual, expected, message) {
  if (typeof expected !== 'object') {
    return strictEqual(actual, expected, message);
  }

  if (!isDeepStrictEqual(actual, expected)) {
    fail(actual, expected, message, '===');
  }
}


function notStrictEqual(actual, expected, message) {
  if (actual === expected) {
    fail(actual, expected, message, '!==');
  }
}


function throws(block, expected, message) {
  var actual;

  try {
    block();
  } catch (e) {
    actual = e;
  }

  if (!actual) {
    fail(actual, expected, 'Missing expected exception: ' + message);
  }
  if (expected && expectedException(actual, expected, message) !== true) {
    throw actual;
  }
}


function doesNotThrow(block, message) {
  var actual;

  try {
    block();
  } catch (e) {
    actual = e;
  }

  message = (message ? ' ' + message : '');

  if (actual) {
    fail(actual, null, 'Got unwanted exception.' + message);
  }
}

function expectedException(actual, expected, msg) {
  if (typeof expected !== 'function') {
    if (util.isRegExp(expected))
      return expected.test(actual);

    // Handle primitives properly.
    if (typeof actual !== 'object' || actual === null) {
      throw new AssertionError({
        actual: actual,
        expected: expected,
        message: msg,
        operator: 'throws',
      });
    }

    var keys = Object.keys(expected);
    // Special handle errors to make sure the name and the message are compared
    // as well.
    if (expected instanceof Error) {
      keys.push('name', 'message');
    }
    for (var idx = 0; idx < keys.length; ++idx) {
      var key = keys[idx];
      if (typeof actual[key] === 'string' &&
          util.isRegExp(expected[key]) &&
          expected[key].test(actual[key])) {
        continue;
      }
      strictEqual(actual[key], expected[key], msg);
    }
    return true;
  }
  // Guard instanceof against arrow functions as they don't have a prototype.
  if (expected.prototype !== undefined && actual instanceof expected) {
    return true;
  }
  if (Error.isPrototypeOf(expected)) {
    return false;
  }
  return expected.call({}, actual) === true;
}

assert.AssertionError = AssertionError;
assert.ok = assert;
assert.fail = fail;
// eslint-disable-next-line no-restricted-properties
assert.equal = equal;
// eslint-disable-next-line no-restricted-properties
assert.notEqual = notEqual;
assert.strictEqual = strictEqual;
assert.deepStrictEqual = deepStrictEqual;
assert.notStrictEqual = notStrictEqual;
assert.throws = throws;
assert.doesNotThrow = doesNotThrow;

module.exports = assert;
