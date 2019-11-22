'use strict';

console.log(process.env);

var assert = require('assert');
var bus = require('dbus').getBus('session');
bus.destroy();

assert.throws(() => {
  require('dbus').getBus('session');
}, Error);
