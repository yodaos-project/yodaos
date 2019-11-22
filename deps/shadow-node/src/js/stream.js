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

var eventEmitter = require('events').EventEmitter;
var util = require('util');

function Stream() {
  eventEmitter.call(this);
}

util.inherits(Stream, eventEmitter);
module.exports = Stream;

Stream.Stream = Stream;
Stream.Readable = require('stream_readable');
Stream.Writable = require('stream_writable');
Stream.Duplex = require('stream_duplex');
Stream.Transform = require('stream_transform');
Stream.PassThrough = require('stream_passthrough');
