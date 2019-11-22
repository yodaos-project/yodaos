### Platform Support

The following shows Assert module APIs available for each platform.

|  | Linux<br/>(Ubuntu) | Raspbian<br/>(Raspberry Pi) | NuttX<br/>(STM32F4-Discovery) | TizenRT<br/>(Artik053) |
| :---: | :---: | :---: | :---: | :---: |
| assert.ok | O | O | O | O |
| assert.doesNotThrow | O | O | O | O |
| assert.equal | O | O | O | O |
| assert.fail | O | O | O | O |
| assert.notEqual | O | O | O | O |
| assert.notStrictEqual | O | O | O | O |
| assert.strictEqual | O | O | O | O |
| assert.throws | O | O | O | O |

# Assert

Assert module is designed for writing tests for applications.

You can access the functions of the module by adding `require('assert')` to your file.


## Class: AssertionError

Assert module will produce `AssertionError` in case of an assertion failure. `AssertionError` inherits standard `Error` thus it has properties provided by `Error` object including additional properties.


* `actual` {any} This property contains the actual value.
* `expected` {any} This property contains the expected value.
* `message` {any} The error message, default value is the error itself.
* `name` {string} The name is `AssertionError` string.
* `operator` {string} This property contains the operator used for comparing `actual` with `expected`.


### assert(value[, message])
* `value` {any} Value to test.
* `message` {any} Message displayed in the thrown error.

An alias of assert.ok().


### assert.ok(value[, message])
* `value` {any} Value to test.
* `message` {any} Message displayed in the thrown error.

Checks if the `value` is truthy. If it is not, throws an AssertionError, with the given optional `message`.

**Example**

```js
var assert = require('assert');

assert.ok(1);
// OK

assert.ok(true);
// OK

assert.ok(false);
// throws "AssertionError: false == true"

assert.ok(0);
// throws "AssertionError: 0 == true"

assert.ok(false, "it's false");
// throws "AssertionError: it's false"
```


### assert.doesNotThrow(block[, message])
* `block` {Function}
* `message` {any} Message to be displayed.

Tests if the given `block` does not throw any exception. Otherwise throws an
exception with the given optional `message`.

**Example**

```js
var assert = require('assert');

assert.doesNotThrow(
  function() {
    assert.ok(1);
  }
);
// OK

assert.doesNotThrow(
  function() {
    assert.ok(0);
  }
)
// throws "AssertionError: Got unwanted exception."
```


### assert.equal(actual, expected[, message])
* `actual` {any} The actual value.
* `expected` {any} The expected value.
* `message` {any} Message to be displayed.

Tests if `actual == expected` is evaluated to `true`. Otherwise throws an
exception with the given optional `message`.

**Example**

```js
var assert = require('assert');

assert.equal(1, 1);
assert.equal(1, '1');
```


### assert.fail(actual, expected, message, operator)
* `actual` {any} The actual value.
* `expected` {any} The expected value.
* `message` {any} Message to be displayed.
* `operator` {string} The operator.

Throws an `AssertionError` exception with the given `message`.

**Example**

```js
var assert = require('assert');

assert.fail(1, 2, undefined, '>');
// AssertionError: 1 > 2
```


### assert.notEqual(actual, expected[, message])
* `actual` {any} The actual value.
* `expected` {any} The expected value.
* `message` {any} Message to be displayed.

Tests if `actual != expected` is evaluated to `true`. Otherwise throws an
exception with the given optional `message`.

**Example**

```js
var assert = require('assert');

assert.notEqual(1, 2);
```


### assert.notStrictEqual(actual, expected[, message])
* `actual` {any} The actual value.
* `expected` {any} The expected value.
* `message` {any} Message to be displayed.

Tests if `actual !== expected` is evaluated to `true`. Otherwise throws an exception
with the given optional `message`.

**Example**

```js
var assert = require('assert');

assert.notStrictEqual(1, 2);
// OK

assert.notStrictEqual(1, 1);
// AssertionError: 1 !== 1

assert.notStrictEqual(1, '1');
// OK
```


### assert.strictEqual(actual, expected[, message])
* `actual` {any} The actual value.
* `expected` {any} The expected value.
* `message` {any} Message to be displayed.

Tests if `actual === expected` is evaluated to `true`. Otherwise throws an exception
with the given optional `message`.

**Example**

```js
var assert = require('assert');

assert.strictEqual(1, 1);
// OK

assert.strictEqual(1, 2);
// AssertionError: 1 === 2

assert.strictEqual(1, '1');
// AssertionError: 1 === '1'
```


### assert.deepStrictEqual(actual, expected, message)
* `actual` {any} The actual value.
* `expected` {any} The expected value.
* `message` {any} Message to be displayed.

**Example**

```js
var assert = require('assert');

var obj1 = { a: 1, b: 2 };
var obj2 = { a: 1, b: 2 };
var obj3 = { a: 1, b: 3 };

assert.deepStrictEqual(obj1, obj2);
// OK

assert.deepStrictEqual(obj1, obj3);
// AssertionError

assert.deepStrictEqual(NaN, NaN);
// OK
```


### assert.throws(block[, expected, message])
* `block` {Function} The function that throws an error.
* `expected` {Function|RegExp|Object|Error} The expected error type.
* `message` {any} Message to be displayed.

Tests if the given `block` throws an `expected` error. Otherwise throws an exception
with the given optional `message`.

**Example**

```js
var assert = require('assert');

assert.throws(
  function() {
    assert.equal(1, 2);
  },
  assert.AssertionError
);
// OK

assert.throws(() => {
  throw new Error('foobar');
}, /foobar/);
// OK

assert.throws(() => {
  var e = new Error('foobar');
  e.code = 'ENO';
  throw e;
}, {
  message: 'foobar',
  code: 'ENO',
});
// OK

assert.throws(() => {
  throw new Error('foobar');
}, err => err.message === 'foobar');
// OK

assert.throws(
  function() {
    assert.equal(1, 1);
  },
  assert.AssertionError
);
// Uncaught error: Missing exception

assert.throws(
  function() {
    assert.equal(1, 2);
  },
  TypeError
);
// AssertionError
```
