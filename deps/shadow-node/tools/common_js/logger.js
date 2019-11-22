/* Copyright 2016-present Samsung Electronics Co., Ltd. and other contributors
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

function Logger(path) {
  this.text_colors = {
    red: '\u001b[1;31m',
    yellow: '\u001b[1;33m',
    green: '\u001b[1;32m',
    blue: '\u001b[1;34m',
    empty: '\u001b[0m',
  };
  this.status = {
    pass: 'pass',
    skip: 'skip',
    fail: 'fail',
    timeout: 'timeout',
    summary: 'summary'
  };
  this.path = path;

  return this;
}

Logger.prototype.message = function(msg, status) {
  if (this.path) {
    // FIXME : After fs.appendFile is implemented, it should be replaced.
    var data = fs.readFileSync(this.path);
    var newData = data + msg + '\n';
    fs.writeFileSync(this.path, new Buffer(newData));
  }
  if (status === this.status.pass) {
    console.log(this.text_colors.green + msg + this.text_colors.empty);
  } else if (status === this.status.skip) {
    console.log(this.text_colors.yellow + msg + this.text_colors.empty);
  } else if (status === this.status.fail || status === this.status.timeout) {
    console.log(this.text_colors.red + msg + this.text_colors.empty);
  } else if (status === this.status.summary) {
    console.log(this.text_colors.blue + msg + this.text_colors.empty);
  } else {
    console.log(msg);
  }
};

module.exports.Logger = Logger;
