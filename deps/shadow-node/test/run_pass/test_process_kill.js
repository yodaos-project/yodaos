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

var common = require('../common');
var assert = require('assert');
var childProcess = require('child_process');

var cp = childProcess.spawn('sleep', [ '1000' ]);
cp.on('exit', common.mustCall((code, signal) => {
  assert(code == null);
  assert.strictEqual(signal, 'SIGINT');
}));
process.kill(cp.pid, /** SIGINT */2);

cp = childProcess.spawn('sleep', [ '1000' ]);
cp.on('exit', common.mustCall((code, signal) => {
  assert(code == null);
  assert.strictEqual(signal, 'SIGINT');
}));
process.kill(cp.pid, 'SIGINT');

cp = childProcess.spawn('sleep', [ '1000' ]);
cp.on('exit', common.mustCall((code, signal) => {
  assert(code == null);
  assert.strictEqual(signal, 'SIGTERM');

  try {
    process.kill(cp.pid, 0);
    assert.fail('unreachable path');
  } catch (err) {
    assert.strictEqual(err.message, 'kill ESRCH(no such process)');
    assert.strictEqual(err.code, 'ESRCH');
  }
}));
process.kill(cp.pid/** , defaults to SIGTERM */);
