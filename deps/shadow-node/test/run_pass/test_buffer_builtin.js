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

var assert = require('assert');


var buff1 = new Buffer('test');
assert.strictEqual(buff1._builtin.toString(0, 0), '');
assert.strictEqual(buff1._builtin.toString(0, 1), 't');
assert.strictEqual(buff1._builtin.toString(0, 2), 'te');
assert.strictEqual(buff1._builtin.toString(0, 3), 'tes');
assert.strictEqual(buff1._builtin.toString(0, 4), 'test');
assert.strictEqual(buff1._builtin.toString(1, 4), 'est');
assert.strictEqual(buff1._builtin.toString(2, 4), 'st');
assert.strictEqual(buff1._builtin.toString(3, 4), 't');
assert.strictEqual(buff1._builtin.toString(4, 4), '');

assert.strictEqual(buff1._builtin.toString(-1, 5), 'test');
assert.strictEqual(buff1._builtin.toString(-1, 2), 'te');
assert.strictEqual(buff1._builtin.toString(2, 5), 'st');


var buff2 = new Buffer(10);
buff2._builtin.write('abcde', 0, 5);
assert.strictEqual(buff2.toString(), 'abcde');
assert.strictEqual(buff2.length, 10);

buff2._builtin.write('fgh', 5, 3);
assert.strictEqual(buff2.toString(), 'abcdefgh');
assert.strictEqual(buff2.length, 10);

buff2._builtin.write('AB', 0, 10);
assert.strictEqual(buff2.toString(), 'ABcdefgh');
assert.strictEqual(buff2.length, 10);

buff2._builtin.write('ab', -1, 11);
assert.strictEqual(buff2.toString(), 'abcdefgh');
assert.strictEqual(buff2.length, 10);

buff2._builtin.write('ijklmnopqrstu', 8, 5);
assert.strictEqual(buff2.toString(), 'abcdefghij');
assert.strictEqual(buff2.length, 10);

buff2._builtin.write('\0\0', 8, 2);
assert.strictEqual(buff2.toString(), 'abcdefgh');
assert.strictEqual(buff2.length, 10);


var buff3 = Buffer.concat([buff1, buff2]);


var buff4 = new Buffer(10);
var buff5 = new Buffer('a1b2c3');
buff5._builtin.copy(buff4, 0, 0, 6);
assert.strictEqual(buff4.toString(), 'a1b2c3');
buff5._builtin.copy(buff4, 4, 2, 6);
assert.strictEqual(buff4.toString(), 'a1b2b2c3');


var buff6 = buff3._builtin.slice(1, buff3.length);
assert.strictEqual(buff6.toString(), 'estabcdefgh');
assert.strictEqual(buff6.length, 13);

var buff7 = buff6._builtin.slice(3, 5);
assert.strictEqual(buff7.toString(), 'ab');
assert.strictEqual(buff7.length, 2);

var buff8 = new Buffer(buff5);
assert.strictEqual(buff8.toString(), 'a1b2c3');
assert.strictEqual(buff8.equals(buff5), true);
assert.strictEqual(buff8.equals(buff6), false);

var buff9 = new Buffer('abcabcabcd');
var buff10 = buff9._builtin.slice(0, 3);
var buff11 = buff9._builtin.slice(3, 6);
var buff12 = buff9._builtin.slice(6, buff9.length);
assert.strictEqual(buff10.equals(buff11), true);
assert.strictEqual(buff11.equals(buff10), true);
assert.strictEqual(buff11.equals(buff12), false);
assert.strictEqual(buff10.compare(buff11), 0);
assert.strictEqual(buff11.compare(buff10), 0);
assert.strictEqual(buff11.compare(buff12), -1);
assert.strictEqual(buff12.compare(buff11), 1);

assert.strictEqual(buff9._builtin.slice(-2, buff9.length).toString(), 'cd');
assert.strictEqual(buff9._builtin.slice(-3, -2).toString(), 'b');
assert.strictEqual(buff9._builtin.slice(0, -2).toString(), 'abcabcab');


assert.strictEqual(buff3.toString(), 'testabcdefgh');


assert.strictEqual(Buffer.byteLength('\u007F'), 1);
assert.strictEqual(Buffer.byteLength('\u008F'), 2);
assert.strictEqual(Buffer.byteLength('\u08FF'), 3);
assert.strictEqual(Buffer.byteLength('abc'), 'abc'.length);
assert.notStrictEqual(Buffer.byteLength('\u2040'), '\u2040'.length);
