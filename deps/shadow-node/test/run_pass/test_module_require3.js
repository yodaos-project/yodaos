'use strict';

var assert = require('assert');
var path = require('path');

(function testInjectFakeModule() {
  var relativePath = '../run_pass/require3/semicolon.js';
  var absolutePath = path.join(__dirname, relativePath);
  var fakeModule = {};
  require.cache[absolutePath] = { exports: fakeModule };
  assert.strictEqual(require(relativePath), fakeModule);
})();

(function testInjectFakeNativeModule() {
  var relativePath = 'fs';
  var fakeModule = {};
  require.cache[relativePath] = { exports: fakeModule };
  assert.strictEqual(require(relativePath), fakeModule);
})();

(function testCachedModuleScope() {
  var relativePath = '../run_pass/require3/foo.js';
  var absolutePath = path.join(__dirname, relativePath);
  require(absolutePath);
  var cached = require.cache.fs;
  assert.strictEqual(require('fs'), cached.exports);

  require.cache.fs = null;
  assert.notStrictEqual(require('fs'), cached.exports);
})();
