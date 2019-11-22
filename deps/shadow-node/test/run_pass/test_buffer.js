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

var buff1 = new Buffer('test');
assert.strictEqual(buff1.toString(), 'test');
assert.strictEqual(buff1.toString(0, 0), '');
assert.strictEqual(buff1.toString(0, 1), 't');
assert.strictEqual(buff1.toString(0, 2), 'te');
assert.strictEqual(buff1.toString(0, 3), 'tes');
assert.strictEqual(buff1.toString(0, 4), 'test');
assert.strictEqual(buff1.toString(1, 4), 'est');
assert.strictEqual(buff1.toString(2, 4), 'st');
assert.strictEqual(buff1.toString(3, 4), 't');
assert.strictEqual(buff1.toString(4, 4), '');
assert.strictEqual(buff1.length, 4);

var buff2 = new Buffer(10);
buff2.write('abcde');
assert.strictEqual(buff2.toString(), 'abcde');
assert.strictEqual(buff2.length, 10);

buff2.write('fgh', 5);
assert.strictEqual(buff2.toString(), 'abcdefgh');
assert.strictEqual(buff2.length, 10);

assert.throws(function() { buff2.write('ijk', -1); }, RangeError);
assert.throws(function() { buff2.write('ijk', 10); }, RangeError);

var buff3 = Buffer.concat([buff1, buff2]);
assert.strictEqual(buff3.toString(), 'testabcdefgh');
assert.strictEqual(buff3.length, 14);

var buff4 = new Buffer(10);
var buff5 = new Buffer('a1b2c3');
buff5.copy(buff4);
assert.strictEqual(buff4.toString(), 'a1b2c3');
buff5.copy(buff4, 4, 2);
assert.strictEqual(buff4.toString(), 'a1b2b2c3');
assert.throws(function() { buff5.copy(buff4, -1); }, RangeError);
assert.throws(function() { buff2.write(null, 0, 0); }, TypeError);
assert.throws(function() { buff5.copy(null); }, TypeError);
assert.throws(function() { buff5.compare(null); }, TypeError);
assert.throws(function() { buff5.equals(null); }, TypeError);
assert.throws(function() { Buffer.concat([buff1, null]); }, TypeError);
assert.throws(function() { Buffer.concat(null, null); }, TypeError);
assert.throws(function() { new Buffer(null); }, TypeError);

new Buffer(-1);

var buff_1 = new Buffer('asd');
var buff_2 = new Buffer(2);
buff_1.copy(buff_2, 0, 0, 2);
buff_2.write('asd', 0, 3);

assert.throws(function() { new Buffer('asd', 'hex'); }, TypeError);

var buff_3 = new Buffer(4);
assert.throws(function() { buff_3.writeUInt8(10000, 0); }, TypeError);
var buff_4 = new Buffer(4);
assert.throws(function() { buff_4.writeUInt8(0x11, 3000); }, RangeError);
var buff_5 = new Buffer(4);
assert.throws(function() { buff_5.readUInt8(3000); }, RangeError);
buff_5.slice(undefined, 2);

buff_5.fill('asd');

buff_5.readInt8(3, true);
buff_5.writeUInt32LE(0, 0, true);
buff_5.writeUInt8(0, 0, true);
buff_5.writeUInt16LE(0, 0, true);
buff_5.writeInt16LE(0, 0, true);
buff_5.writeInt32LE(0, 0, true);


var buff6 = buff3.slice(1);
assert.strictEqual(buff6.toString(), 'estabcdefgh');
assert.strictEqual(buff6.length, 13);

var buff7 = buff6.slice(3, 5);
assert.strictEqual(buff7.toString(), 'ab');
assert.strictEqual(buff7.length, 2);

var buff8 = new Buffer(buff5);
assert.strictEqual(buff8.toString(), 'a1b2c3');
assert.strictEqual(buff8.equals(buff5), true);
assert.strictEqual(buff8.equals(buff6), false);

var buff9 = new Buffer('abcabcabcd');
var buff10 = buff9.slice(0, 3);
var buff11 = buff9.slice(3, 6);
var buff12 = buff9.slice(6);
assert.strictEqual(buff10.equals(buff11), true);
assert.strictEqual(buff11.equals(buff10), true);
assert.strictEqual(buff11.equals(buff12), false);
assert.strictEqual(buff10.compare(buff11), 0);
assert.strictEqual(buff11.compare(buff10), 0);
assert.strictEqual(buff11.compare(buff12), -1);
assert.strictEqual(buff12.compare(buff11), 1);

assert.strictEqual(buff9.slice(-2).toString(), 'cd');
assert.strictEqual(buff9.slice(-3, -2).toString(), 'b');
assert.strictEqual(buff9.slice(0, -2).toString(), 'abcabcab');


assert.strictEqual(Buffer.isBuffer(buff9), true);
assert.strictEqual(Buffer.isBuffer(1), false);
assert.strictEqual(Buffer.isBuffer({}), false);
assert.strictEqual(Buffer.isBuffer('1'), false);
assert.strictEqual(Buffer.isBuffer([1]), false);
assert.strictEqual(Buffer.isBuffer([buff1]), false);
assert.strictEqual(Buffer.isBuffer({ obj: buff1 }), false);


assert.strictEqual(buff3.toString(), 'testabcdefgh');


var buff13 = new Buffer(4);
buff13.writeUInt8(0x11, 0);
assert.strictEqual(buff13.readUInt8(0), 0x11);

buff13.writeUInt16LE(0x3456, 1);
assert.strictEqual(buff13.readUInt8(1), 0x56);
assert.strictEqual(buff13.readUInt8(2), 0x34);
assert.strictEqual(buff13.readUInt16LE(1), 0x3456);

buff13.writeUInt32LE(0x89abcdef, 0);
assert.strictEqual(buff13.readUInt8(0), 0xef);
assert.strictEqual(buff13.readUInt8(1), 0xcd);
assert.strictEqual(buff13.readUInt8(2), 0xab);
assert.strictEqual(buff13.readUInt8(3), 0x89);
assert.strictEqual(buff13.readUInt16LE(0), 0xcdef);
assert.strictEqual(buff13.readUInt16LE(2), 0x89ab);

buff13.writeUInt16LE(0x0102, 0);
assert.strictEqual(buff13.readUInt8(0), 0x02);
assert.strictEqual(buff13.readInt8(0), 2);
assert.strictEqual(buff13.readUInt8(1), 0x01);
assert.strictEqual(buff13.readInt8(1), 1);
assert.strictEqual(buff13.readUInt16LE(0), 0x0102);
assert.strictEqual(buff13.readInt16LE(0), 258);

buff13.writeInt32LE(0x7fabcdef, 0);
assert.strictEqual(buff13.readUInt8(0), 0xef);
assert.strictEqual(buff13.readInt8(0), -17);
assert.strictEqual(buff13.readUInt8(1), 0xcd);
assert.strictEqual(buff13.readInt8(1), -51);
assert.strictEqual(buff13.readUInt8(2), 0xab);
assert.strictEqual(buff13.readInt8(2), -85);
assert.strictEqual(buff13.readInt8(3), 0x7f);
assert.strictEqual(buff13.readUInt16LE(0), 0xcdef);
assert.strictEqual(buff13.readInt16LE(0), -12817);
assert.strictEqual(buff13.readInt16LE(2), 0x7fab);

var buff14 = new Buffer([0x01, 0xa1, 0xfb]);
assert.strictEqual(buff14.readUInt8(0), 0x01);
assert.strictEqual(buff14.readUInt8(1), 0xa1);
assert.strictEqual(buff14.readUInt8(2), 0xfb);
assert.strictEqual(buff14.readInt8(2), -5);
assert.strictEqual(buff14.toString('hex'), '01a1fb');

var buff15 = new Buffer('7456a9', 'hex');
assert.strictEqual(buff15.length, 3);
assert.strictEqual(buff15.readUInt8(0), 0x74);
assert.strictEqual(buff15.readUInt8(1), 0x56);
assert.strictEqual(buff15.readUInt8(2), 0xa9);
assert.strictEqual(buff15.toString('hex'), '7456a9');

var buff16 = new Buffer(4);
var ret = buff16.fill(7);
assert.strictEqual(buff16.readInt8(0), 7);
assert.strictEqual(buff16.readInt8(1), 7);
assert.strictEqual(buff16.readInt8(2), 7);
assert.strictEqual(buff16.readInt8(3), 7);
assert.strictEqual(buff16, ret);
buff16.fill(13 + 1024);
assert.strictEqual(buff16.readInt8(0), 13);
assert.strictEqual(buff16.readInt8(1), 13);
assert.strictEqual(buff16.readInt8(2), 13);
assert.strictEqual(buff16.readInt8(3), 13);

assert.strictEqual(Buffer(new Array()).toString(), '');
assert.strictEqual(new Buffer(1).readUInt8(1, true), 0);
assert.strictEqual(new Buffer(1).readUInt16LE({}, true), 0);

var buff17 = new Buffer('a');
assert.throws(function() { buff17.fill(8071).toString(); }, TypeError);

var buff18 = new Buffer(4);
buff18.fill(7);
assert.strictEqual(buff18.readInt16LE(0), 1799);
assert.strictEqual(buff18.readInt32LE(0), 117901063);

var buff19 = new Buffer([0xb5, 0x01]);
assert.strictEqual(buff19.readInt16LE(0), 437);

var buff20 = new Buffer([10, 20, 30, 40]);
assert.strictEqual(buff20.readFloatLE(0), 8.775107173558151e-15);
assert.throws(function() {
  var buff = new Buffer(3);
  buff.fill(10);
  buff.readFloatLE(0);
}, RangeError);

var buff21 = new Buffer([10, 20, 30, 40, 0, 0, 10, 20]);
assert.strictEqual(buff21.readDoubleLE(0), 3.8615925991830683e-212);
assert.throws(function() {
  var buff = new Buffer(7);
  buff.fill(10);
  buff.readDoubleLE(0);
}, RangeError);
