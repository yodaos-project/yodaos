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

var util = require('util');
var assert = require('assert');

assert.strictEqual(util.isNull(null), true);
assert.strictEqual(util.isNull(0), false);
assert.strictEqual(util.isNull('null'), false);


assert.strictEqual(util.isUndefined(undefined), true);
assert.strictEqual(util.isUndefined(0), false);
assert.strictEqual(util.isUndefined('undefined'), false);


assert.strictEqual(util.isNumber(0), true);
assert.strictEqual(util.isNumber(5001), true);
assert.strictEqual(util.isNumber(3.14), true);
assert.strictEqual(util.isNumber(-5001), true);
assert.strictEqual(util.isNumber('5000'), false);
assert.strictEqual(util.isNumber(null), false);
assert.strictEqual(util.isNumber([0, 1, 2, 3, 4, 5]), false);


assert.strictEqual(util.isFinite(5001), true);
assert.strictEqual(util.isFinite(-5001), true);
assert.strictEqual(util.isFinite(3.14), true);
assert.strictEqual(util.isFinite(-3.14), true);
assert.strictEqual(util.isFinite(0), true);
assert.strictEqual(util.isFinite(Infinity), false);


assert.strictEqual(util.isBoolean(true), true);
assert.strictEqual(util.isBoolean(false), true);
assert.strictEqual(util.isBoolean(Boolean(5001)), true);
assert.strictEqual(util.isBoolean(Boolean(0)), true);
assert.strictEqual(util.isBoolean(Boolean(-5001)), true);
assert.strictEqual(util.isBoolean(Boolean('Hello IoT.js')), true);


assert.strictEqual(util.isString('Hello IoT.js'), true);
assert.strictEqual(util.isString(['Hello IoT.js']), false);
assert.strictEqual(util.isString(-5001), false);
assert.strictEqual(util.isString(5001), false);
assert.strictEqual(util.isString(null), false);


var object = {
  value1: 0,
  value2: 5001,
  value3: -5001,
  value4: 3.14,
  value5: 'Hello IoT.js'
};
assert.strictEqual(util.isObject(object), true);
assert.strictEqual(util.isObject({ obj: 5001 }), true);
assert.strictEqual(util.isObject({ obj: object }), true);
assert.strictEqual(util.isObject({}), true);
assert.strictEqual(util.isObject([5001]), true);
assert.strictEqual(util.isObject(['Hello IoT.js']), true);
assert.strictEqual(util.isObject(null), false);
assert.strictEqual(util.isObject(5001), false);
assert.strictEqual(util.isObject('Hello IoT.js'), false);


function func1() {}
assert.strictEqual(util.isFunction(func1), true);
assert.strictEqual(util.isFunction(function(arg) { /* do nothing */ }), true);
assert.strictEqual(util.isFunction(null), false);
assert.strictEqual(util.isFunction(5001), false);
assert.strictEqual(util.isFunction([5001]), false);
assert.strictEqual(util.isFunction([func1]), false);
assert.strictEqual(util.isFunction({}), false);
assert.strictEqual(util.isFunction('Hello IoT.js'), false);


var buff = new Buffer('Hello IoT.js');
assert.strictEqual(util.isBuffer(buff), true);
assert.strictEqual(util.isBuffer(new Buffer(5001)), true);
assert.strictEqual(util.isBuffer(5001), false);
assert.strictEqual(util.isBuffer({}), false);
assert.strictEqual(util.isBuffer('5001'), false);
assert.strictEqual(util.isBuffer([5001]), false);
assert.strictEqual(util.isBuffer([buff]), false);
assert.strictEqual(util.isBuffer({ obj: buff }), false);


function Parent() {}
function Child() {}
util.inherits(Child, Parent);
var child = new Child();
assert.strictEqual(child instanceof Parent, true);
assert.strictEqual(child instanceof Buffer, false);


assert.strictEqual(util.format(), '');
assert.strictEqual(util.format(''), '');
assert.strictEqual(util.format(null), 'null');
assert.strictEqual(util.format(true), 'true');
assert.strictEqual(util.format(false), 'false');
assert.strictEqual(util.format('Hello IoT.js'), 'Hello IoT.js');
assert.strictEqual(util.format(5001), '5001');

assert.strictEqual(util.format('%d', 5001.5), '5001.5');
assert.strictEqual(util.format('Hello IoT.js - %d', 5001),
                   'Hello IoT.js - 5001');
assert.strictEqual(
  util.format('%s IoT.js - %d', 'Hello', 5001),
  'Hello IoT.js - 5001'
);
assert.strictEqual(util.format('%d%%', 5001), '5001%');

var json = {
  'first': '1st',
  'second': '2nd'
};
assert.strictEqual(
  util.format('%s: %j', 'Object', json),
  'Object: {"first":"1st","second":"2nd"}'
);
assert.strictEqual(
  util.format('%d-%j-%s', 5001, json, 'IoT.js', 'end'),
  '5001-{"first":"1st","second":"2nd"}-IoT.js end'
);
json.json = json;
assert.strictEqual(util.format('%j', json), '[Circular]');

assert.strictEqual(util.format('%s', '5001'), '5001');
assert.strictEqual(util.format('%j', '5001'), '"5001"');
assert.strictEqual(util.format('%d%d', 5001), '5001%d');
assert.strictEqual(util.format('%s%d%s%d', 'IoT.js ', 5001), 'IoT.js 5001%s%d');
assert.strictEqual(util.format('%d%% %s', 100, 'IoT.js'), '100% IoT.js');
assert(util.format(new Error('format')).match(/^Error: format/));

var err1 = util.errnoException(3008, 'syscall', 'original message');
assert.strictEqual(err1 instanceof Error, true);
assert.strictEqual(String(err1), 'Error: syscall error original message');
assert.strictEqual(err1.code, 'error');
assert.strictEqual(err1.errno, 'error');
assert.strictEqual(err1.syscall, 'syscall');

var err2 = util.errnoException(1, 'getSyscall');
assert.strictEqual(err2 instanceof Error, true);
assert.strictEqual(String(err2), 'Error: getSyscall error');
assert.strictEqual(err2.code, 'error');
assert.strictEqual(err2.errno, 'error');
assert.strictEqual(err2.syscall, 'getSyscall');


var err3 = util.exceptionWithHostPort(1,
                                      'syscall',
                                      '127.0.0.1',
                                      5001,
                                      'additional info');
assert.strictEqual(err3 instanceof Error, true);
assert.strictEqual(
  String(err3),
  'Error: syscall error 127.0.0.1:5001 - Local (additional info)'
);
assert.strictEqual(err3.code, 'error');
assert.strictEqual(err3.errno, 'error');
assert.strictEqual(err3.syscall, 'syscall');
assert.strictEqual(err3.address, '127.0.0.1');
assert.strictEqual(err3.port, 5001);

var err4 = util.exceptionWithHostPort(3008,
                                      'getSyscall',
                                      '127.0.0.1');
assert.strictEqual(err4 instanceof Error, true);
assert.strictEqual(
  String(err4),
  'Error: getSyscall error 127.0.0.1'
);
assert.strictEqual(err4.code, 'error');
assert.strictEqual(err4.errno, 'error');
assert.strictEqual(err4.syscall, 'getSyscall');
assert.strictEqual(err4.address, '127.0.0.1');
