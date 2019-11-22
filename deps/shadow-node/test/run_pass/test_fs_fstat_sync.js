/* Copyright 2017-present Samsung Electronics Co., Ltd. and other contributors
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

var testfile = process.cwd() + '/run_pass/test_fs_fstat_sync.js';
var testdir = process.cwd() + '/resources';
var flags = 'r';


// fstatSync - file
try {
  var fd = fs.openSync(testfile, flags);
  var stat = fs.fstatSync(fd);

  assert.strictEqual(stat.isFile(), true);
  assert.strictEqual(stat.isDirectory(), false);
  fs.closeSync(fd);
} catch (err) {
  throw err;
}


// fstatSync - directory
try {
  fd = fs.openSync(testdir, flags);
  stat = fs.fstatSync(fd);

  assert.strictEqual(stat.isFile(), false);
  assert.strictEqual(stat.isDirectory(), true);
  fs.closeSync(fd);
} catch (err) {
  throw err;
}
