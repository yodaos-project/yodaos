'use strict';

var assert = require('assert');
var dbus = require('dbus');

assert.strictEqual(dbus.Define(String), 's');
assert.strictEqual(dbus.Define(Number), 'd');
assert.strictEqual(dbus.Define(Boolean), 'b');
assert.strictEqual(dbus.Define(Array), 'av');
assert.strictEqual(dbus.Define(Object), 'a{sv}');
