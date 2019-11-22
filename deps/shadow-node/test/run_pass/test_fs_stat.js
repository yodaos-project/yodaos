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

var fs = require('fs');
var assert = require('assert');


var stats1 = fs.statSync(process.cwd() + '/run_pass/test_fs_stat.js');
assert.strictEqual(stats1.isFile(), true);
assert.strictEqual(stats1.isDirectory(), false);

fs.stat(process.cwd() + '/run_pass/test_fs_stat.js', function(err, stats) {
  if (!err) {
    assert.strictEqual(stats.isFile(), true);
    assert.strictEqual(stats.isDirectory(), false);
  } else {
    throw err;
  }
});


var stats2 = fs.statSync(process.cwd() + '/resources');
assert.strictEqual(stats2.isDirectory(), true);
assert.strictEqual(stats2.isFile(), false);

fs.stat(process.cwd() + '/resources', function(err, stats) {
  if (!err) {
    assert.strictEqual(stats.isDirectory(), true);
    assert.strictEqual(stats.isFile(), false);
  } else {
    throw err;
  }
});

// fs.statSync throws an exception for a non-existing file.
try {
  fs.statSync(process.cwd() + '/non_existing.js');
  assert.ok(false);
} catch (e) {
  assert.strictEqual(e instanceof Error, true);
  assert.strictEqual(e instanceof assert.AssertionError, false);
}
