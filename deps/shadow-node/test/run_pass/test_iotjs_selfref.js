'use strict';
/* eslint-disable func-style,func-name-matching */
var assert = require('assert');

var foo = function bar() {
  assert(foo === bar);
};

foo();

var globalfunc;

function foobar(func) {
  globalfunc = func;
}

foobar(function bar() {
  assert(globalfunc === bar);
});
