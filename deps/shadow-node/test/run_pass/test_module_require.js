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

var assert = require('assert');

var dir = process.cwd() + '/run_pass/require1/';

// Load a JS file.
var x = require(dir + 'require_add');
assert.strictEqual(x.add(1, 4), 5);

// Load a package.
var pkg1 = require(dir + 'test_pkg');
assert.strictEqual(pkg1.add(22, 44), 66);
assert.strictEqual(pkg1.multi(22, 44), 968);
assert.strictEqual(pkg1.add2(22, 44), 66);

var pkg2 = require(dir + 'test_index');
assert.strictEqual(pkg2.add(22, 44), 66);
assert.strictEqual(pkg2.multi(22, 44), 968);
assert.strictEqual(pkg2.add2(22, 44), 66);

var pkg3 = require(dir + 'test_index2');
assert.strictEqual(pkg3.add(22, 44), 66);
assert.strictEqual(pkg3.multi(22, 44), 968);
assert.strictEqual(pkg3.add2(22, 44), 66);

// Load invalid modules.
assert.throws(function() {
  require(dir + 'babel-template');
}, Error);

// Load arbitrary file.
assert.throws(function() {
  require(dir + 'arbitrary_file.txt');
}, SyntaxError);

assert.throws(function() {
  require('tmp');
}, Error);

// Load empty module.
assert.deepStrictEqual(require(dir + 'empty_module'), {});

// Load not exist module.
assert.throws(function() {
  require('not_exist_module');
}, Error);
