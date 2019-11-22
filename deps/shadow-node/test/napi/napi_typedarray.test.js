'use strict';
var assert = require('assert');

// Testing api calls for arrays
var test_typedarray = require('./build/Release/napi_typedarray.node');

var byteArray = new Uint8Array(3);
byteArray[0] = 0;
byteArray[1] = 1;
byteArray[2] = 2;
assert.strictEqual(byteArray.length, 3);

var doubleArray = new Float64Array(3);
doubleArray[0] = 0.0;
doubleArray[1] = 1.1;
doubleArray[2] = 2.2;
assert.strictEqual(doubleArray.length, 3);

var byteResult = test_typedarray.Multiply(byteArray, 3);
assert.ok(byteResult instanceof Uint8Array);
assert.strictEqual(byteResult.length, 3);
assert.strictEqual(byteResult[0], 0);
assert.strictEqual(byteResult[1], 3);
assert.strictEqual(byteResult[2], 6);

var doubleResult = test_typedarray.Multiply(doubleArray, -3);
assert.ok(doubleResult instanceof Float64Array);
assert.strictEqual(doubleResult.length, 3);
assert.strictEqual(doubleResult[0], -0);
assert.strictEqual(Math.round(10 * doubleResult[1]) / 10, -3.3);
assert.strictEqual(Math.round(10 * doubleResult[2]) / 10, -6.6);

var externalResult = test_typedarray.External();
assert.ok(externalResult instanceof Int8Array);
assert.strictEqual(externalResult.length, 3);
assert.strictEqual(externalResult[0], 0);
assert.strictEqual(externalResult[1], 1);
assert.strictEqual(externalResult[2], 2);
