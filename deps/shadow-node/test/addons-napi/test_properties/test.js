'use strict';
var common = require('../../common');
var assert = require('assert');
var readonlyErrorRE =
  /^TypeError: Cannot assign to read only property '.*' of object '#<Object>'$/;

// Testing api calls for defining properties
var test_object = require(`./build/Release/test_properties.node`);

assert.strictEqual(test_object.echo('hello'), 'hello');

test_object.readwriteValue = 1;
assert.strictEqual(test_object.readwriteValue, 1);
test_object.readwriteValue = 2;
assert.strictEqual(test_object.readwriteValue, 2);

assert.throws(() => { test_object.readonlyValue = 3; }, readonlyErrorRE);

assert.ok(test_object.hiddenValue);

// Properties with napi_enumerable attribute should be enumerable.
var propertyNames = [];
for (var name in test_object) {
  propertyNames.push(name);
}
assert.ok(propertyNames.includes('echo'));
assert.ok(propertyNames.includes('readwriteValue'));
assert.ok(propertyNames.includes('readonlyValue'));
assert.ok(!propertyNames.includes('hiddenValue'));
assert.ok(propertyNames.includes('NameKeyValue'));
assert.ok(!propertyNames.includes('readwriteAccessor1'));
assert.ok(!propertyNames.includes('readwriteAccessor2'));
assert.ok(!propertyNames.includes('readonlyAccessor1'));
assert.ok(!propertyNames.includes('readonlyAccessor2'));

// validate property created with symbol
var start = 'Symbol('.length;
var end = start + 'NameKeySymbol'.length;
var symbolDescription =
    String(Object.getOwnPropertySymbols(test_object)[0]).slice(start, end);
assert.strictEqual(symbolDescription, 'NameKeySymbol');

// The napi_writable attribute should be ignored for accessors.
test_object.readwriteAccessor1 = 1;
assert.strictEqual(test_object.readwriteAccessor1, 1);
assert.strictEqual(test_object.readonlyAccessor1, 1);
assert.throws(() => { test_object.readonlyAccessor1 = 3; }, readonlyErrorRE);
test_object.readwriteAccessor2 = 2;
assert.strictEqual(test_object.readwriteAccessor2, 2);
assert.strictEqual(test_object.readonlyAccessor2, 2);
assert.throws(() => { test_object.readonlyAccessor2 = 3; }, readonlyErrorRE);

assert.strictEqual(test_object.hasNamedProperty(test_object, 'echo'), true);
assert.strictEqual(test_object.hasNamedProperty(test_object, 'hiddenValue'),
                   true);
assert.strictEqual(test_object.hasNamedProperty(test_object, 'doesnotexist'),
                   false);
