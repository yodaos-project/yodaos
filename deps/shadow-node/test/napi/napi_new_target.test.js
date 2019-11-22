'use strict';

var assert = require('assert');
var util = require('util');
var binding = require('./build/Release/napi_new_target.node');

function Class() {
  binding.BaseClass.call(this);
  this.method();
}
util.inherits(Class, binding.BaseClass);

Class.prototype.method = function method() {
  this.ok = true;
};

assert.ok(new Class() instanceof binding.BaseClass);
assert.ok(new Class().ok);
assert.ok(binding.OrdinaryFunction());
assert.ok(
  new binding.Constructor(binding.Constructor) instanceof binding.Constructor);
