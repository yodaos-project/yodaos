'use strict';

var common = require('../../common');
var assert = require('assert');
var binding = require(`./build/Release/binding.node`);

class Class extends binding.BaseClass {
  constructor() {
    super();
    this.method();
  }
  method() {
    this.ok = true;
  }
}

assert.ok(new Class() instanceof binding.BaseClass);
assert.ok(new Class().ok);
assert.ok(binding.OrdinaryFunction());
assert.ok(
  new binding.Constructor(binding.Constructor) instanceof binding.Constructor);
