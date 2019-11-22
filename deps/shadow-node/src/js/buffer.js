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

var util = require('util');
var INSPECT_MAX_BYTES = 50;


function checkInt(buffer, value, offset, ext, max, min) {
  if (value > max || value < min)
    throw new TypeError('value is out of bounds');
  if (offset + ext > buffer.length)
    throw new RangeError('index out of range');
}


function checkOffset(offset, ext, length) {
  if (offset + ext > length)
    throw new RangeError('index out of range');
}

// Buffer constructor
// [1] new Buffer(size)
// [2] new Buffer(buffer)
// [3] new Buffer(string)
// [4] new Buffer(string, encoding)
// [5] new Buffer(array)
function Buffer(subject, encoding) {
  if (!util.isBuffer(this)) {
    return new Buffer(subject, encoding);
  }

  if (util.isNumber(subject)) {
    this.length = subject > 0 ? subject >>> 0 : 0;
  } else if (util.isString(subject)) {
    this.length = Buffer.byteLength(subject, encoding);
  } else if (util.isBuffer(subject) || util.isArray(subject)) {
    this.length = subject.length;
  } else {
    throw new TypeError('Bad arguments: Buffer(string|number|Buffer|Array)');
  }

  this._builtin = new native(this, this.length);

  if (util.isString(subject)) {
    if (encoding !== undefined && util.isString(encoding)) {
      switch (encoding) {
        case 'hex':
          if (this._builtin.hexWrite(
            subject, 0, this.length) !== this.length) {
            throw new TypeError('Invalid hex string');
          }
          break;
        case 'base64':
          if (this._builtin.base64Write(
            subject, 0, this.length) !== this.length) {
            throw new TypeError('Invalid base64 string');
          }
          break;
        default:
          // defaults to utf8 encoding
          // TODO: support ascii, latin1, utf16le encodings
          this.write(subject);
      }
    } else {
      this.write(subject);
    }
  } else if (util.isBuffer(subject)) {
    subject.copy(this);
  } else if (util.isArray(subject)) {
    for (var i = 0; i < this.length; ++i) {
      this._builtin.writeUInt8(subject[i], i);
    }
  }
}


// Buffer.byteLength(string, encoding)
Buffer.byteLength = function(str, encoding) {
  var len = native.byteLength(str, encoding);
  if (encoding !== undefined && util.isString(encoding)) {
    switch (encoding) {
      case 'hex':
        return len >>> 1;
    }
  }
  return len;
};


// Buffer.concat(list)
Buffer.concat = function(list) {
  if (!util.isArray(list)) {
    throw new TypeError('Bad arguments: Buffer.concat([Buffer])');
  }

  var length = 0;
  for (var i = 0; i < list.length; ++i) {
    if (!util.isBuffer(list[i])) {
      throw new TypeError('Bad arguments: Buffer.concat([Buffer])');
    }
    length += list[i].length;
  }

  var buffer = new Buffer(length);
  var pos = 0;
  for (var i0 = 0; i0 < list.length; ++i0) {
    list[i0].copy(buffer, pos);
    pos += list[i0].length;
  }

  return buffer;
};


// Buffer.isBuffer(object)
Buffer.isBuffer = function(object) {
  return util.isBuffer(object);
};


// buffer.equals(otherBuffer)
Buffer.prototype.equals = function(otherBuffer) {
  if (!util.isBuffer(otherBuffer)) {
    throw new TypeError('Bad arguments: buffer.equals(Buffer)');
  }

  return this._builtin.compare(otherBuffer._builtin) === 0;
};


// buffer.compare(otherBuffer)
Buffer.prototype.compare = function(otherBuffer) {
  if (!util.isBuffer(otherBuffer)) {
    throw new TypeError('Bad arguments: buffer.compare(Buffer)');
  }

  return this._builtin.compare(otherBuffer._builtin);
};


// buffer.copy(target[, targetStart[, sourceStart[, sourceEnd]]])
// [1] buffer.copy(target)
// [2] buffer.copy(target, targetStart)
// [3] buffer.copy(target, targetStart, sourceStart)
// [4] buffer.copy(target, targetStart, sourceStart, sourceEnd)
// * targetStart - default to 0
// * sourceStart - default to 0
// * sourceEnd - default to buffer.length
Buffer.prototype.copy = function(target, targetStart, sourceStart, sourceEnd) {
  if (!util.isBuffer(target)) {
    throw new TypeError('Bad arguments: buff.copy(Buffer)');
  }

  targetStart = targetStart === undefined ? 0 : ~~targetStart;
  sourceStart = sourceStart === undefined ? 0 : ~~sourceStart;
  sourceEnd = sourceEnd === undefined ? this.length : ~~sourceEnd;

  if ((sourceEnd > sourceStart) && (targetStart < 0)) {
    throw new RangeError('Attempt to write outside buffer bounds');
  }

  return this._builtin.copy(target, targetStart, sourceStart, sourceEnd);
};


// buffer.write(string[, offset[, length]])
// [1] buffer.write(string)
// [2] buffer.write(string, offset)
// [3] buffer.write(string, offset, length)
// * offset - default to 0
// * length - default to buffer.length - offset
Buffer.prototype.write = function(string, offset, length) {
  if (!util.isString(string)) {
    throw new TypeError('Bad arguments: buff.write(string)');
  }

  offset = offset === undefined ? 0 : ~~offset;
  if (string.length > 0 && (offset < 0 || offset >= this.length)) {
    throw new RangeError('Attempt to write outside buffer bounds');
  }

  var remaining = this.length - offset;
  length = length === undefined ? remaining : ~~length;

  return this._builtin.write(string, offset, length);
};


// buff.slice([start[, end]])
// [1] buff.slice()
// [2] buff.slice(start)
// [3] buff.slice(start, end)
// * start - default to 0
// * end - default to buff.length
Buffer.prototype.slice = function(start, end) {
  start = start === undefined ? 0 : ~~start;
  end = end === undefined ? this.length : ~~end;

  return this._builtin.slice(start, end);
};

// buff.inspect()
Buffer.prototype.inspect = function() {
  var str = '';
  str = this.toString(
    'hex', 0, INSPECT_MAX_BYTES).replace(/(.{2})/g, '$1 ').trim();
  if (this.length > INSPECT_MAX_BYTES)
    str += ' ... ';
  return util.format('<Buffer %s>', str);
};

// buffer.indexOf('\n')
// buffer.indexOf('\n', 10)
Buffer.prototype.indexOf = function(value, byteOffset) {
  try {
    var str = this.slice(byteOffset || undefined).toString('utf8');
    return str.indexOf(value + '');
  } catch (err) {
    throw new Error('Buffer.prototype.indexOf only supports for UTF-8 buffers');
  }
};

// buff.toString([encoding,[,start[, end]]])
// [1] buff.toString()
// [2] buff.toString(start)
// [3] buff.toString(start, end)
// [4] buff.toString('hex')
// * start - default to 0
// * end - default to buff.length
Buffer.prototype.toString = function(encoding, start, end) {
  var buf;
  if (encoding === 'hex' || encoding === 'base64') {
    if (encoding === 'hex')
      buf = this._builtin.toHexString();
    else if (encoding === 'base64')
      buf = this._builtin.toBase64();
    return buf.slice(start || 0, end);
  } else {
    // normalize the arguments.
    if (typeof encoding === 'number') {
      end = start;
      start = encoding;
    }
    start = start === undefined ? 0 : ~~start;
    end = end === undefined ? this.length : ~~end;
    return this._builtin.toString(start, end);
  }
};


// buff.writeUInt8(value, offset[,noAssert])
// [1] buff.writeUInt8(value, offset)
// [2] buff.writeUInt8(value, offset, noAssert)
Buffer.prototype.writeUInt8 = function(value, offset, noAssert) {
  value = +value;
  offset = offset >>> 0;
  if (!noAssert)
    checkInt(this, value, offset, 1, 0xff, 0);
  this._builtin.writeUInt8(value & 0xff, offset);
  return offset + 1;
};


// buff.writeUInt16LE(value, offset[,noAssert])
// [1] buff.writeUInt16LE(value, offset)
// [2] buff.writeUInt16LE(value, offset, noAssert)
Buffer.prototype.writeUInt16LE = function(value, offset, noAssert) {
  value = +value;
  offset = offset >>> 0;
  if (!noAssert)
    checkInt(this, value, offset, 2, 0xffff, 0);
  this._builtin.writeUInt8(value & 0xff, offset);
  this._builtin.writeUInt8((value >>> 8) & 0xff, offset + 1);
  return offset + 2;
};


// buff.writeUInt32LE(value, offset[,noAssert])
// [1] buff.writeUInt32LE(value, offset)
// [2] buff.writeUInt32LE(value, offset, noAssert)
Buffer.prototype.writeUInt32LE = function(value, offset, noAssert) {
  value = +value;
  offset = offset >>> 0;
  if (!noAssert)
    checkInt(this, value, offset, 4, 0xffffffff, 0);
  this._builtin.writeUInt8((value >>> 24) & 0xff, offset + 3);
  this._builtin.writeUInt8((value >>> 16) & 0xff, offset + 2);
  this._builtin.writeUInt8((value >>> 8) & 0xff, offset + 1);
  this._builtin.writeUInt8(value & 0xff, offset);
  return offset + 4;
};


// buff.writeInt16LE(value, offset[,noAssert])
// [1] buff.writeInt16LE(value, offset)
// [2] buff.writeInt16LE(value, offset, noAssert)
Buffer.prototype.writeInt16LE = function(value, offset, noAssert) {
  value = +value;
  offset = offset >>> 0;
  if (!noAssert) {
    checkInt(this, value, offset, 2, 0x7fff, -0x8000);
  }
  this._builtin.writeUInt8(value & 0xff, offset);
  this._builtin.writeUInt8((value >>> 8) && 0xff, offset + 1);
  return offset + 2;
};


// buff.writeInt32LE(value, offset[,noAssert])
// [1] buff.writeInt32LE(value, offset)
// [2] buff.writeInt32LE(value, offset, noAssert)
Buffer.prototype.writeInt32LE = function(value, offset, noAssert) {
  value = +value;
  offset = offset >>> 0;
  if (!noAssert)
    checkInt(this, value, offset, 4, 0x7fffffff, -0x80000000);
  this._builtin.writeUInt8(value & 0xff, offset);
  this._builtin.writeUInt8((value >>> 8) & 0xff, offset + 1);
  this._builtin.writeUInt8((value >>> 16) & 0xff, offset + 2);
  this._builtin.writeUInt8((value >>> 24) & 0xff, offset + 3);
  return offset + 4;
};

// buff.writeInt32BE(value, offset[,noAssert])
// [1] buff.writeInt32BE(value, offset)
// [2] buff.writeInt32BE(value, offset, noAssert)
Buffer.prototype.writeInt32BE = function(value, offset, noAssert) {
  value = +value;
  offset = offset >>> 0;
  if (!noAssert)
    checkInt(this, value, offset, 4, 0x7fffffff, -0x80000000);
  this._builtin.writeUInt8(value & 0xff, offset + 3);
  this._builtin.writeUInt8((value >>> 8) & 0xff, offset + 2);
  this._builtin.writeUInt8((value >>> 16) & 0xff, offset + 1);
  this._builtin.writeUInt8((value >>> 24) & 0xff, offset);
  return offset + 4;
};


// buff.readUInt8(offset[,noAssert])
// [1] buff.readUInt8(offset)
// [2] buff.readUInt8(offset, noAssert)
Buffer.prototype.readUInt8 = function(offset, noAssert) {
  offset = offset >>> 0;
  if (!noAssert)
    checkOffset(offset, 1, this.length);
  return this._builtin.readUInt8(offset);
};


// buff.readInt8(offset[,noAssert])
// [1] buff.readInt8(offset)
// [2] buff.readInt8(offset, noAssert)
Buffer.prototype.readInt8 = function(offset, noAssert) {
  offset = offset >>> 0;
  if (!noAssert)
    checkOffset(offset, 1, this.length);
  var val = this._builtin.readUInt8(offset);
  return !(val & 0x80) ? val : (0xff - val + 1) * -1;
};


// buff.readUInt16LE(offset[,noAssert])
// [1] buff.readUInt16LE(offset)
// [2] buff.readUInt16LE(offset, noAssert)
Buffer.prototype.readUInt16LE = function(offset, noAssert) {
  offset = offset >>> 0;
  if (!noAssert)
    checkOffset(offset, 2, this.length);
  return this._builtin.readUInt8(offset) |
         (this._builtin.readUInt8(offset + 1) << 8);
};


// buff.readInt16LE(offset[,noAssert])
// [1] buff.readInt16LE(offset)
// [2] buff.readInt16LE(offset, noAssert)
Buffer.prototype.readInt16LE = function(offset, noAssert) {
  offset = offset >>> 0;
  if (!noAssert)
    checkOffset(offset, 2, this.length);

  var val = this._builtin.readUInt8(offset) |
    (this._builtin.readUInt8(offset + 1) << 8);
  return (val & 0x8000) ? val | 0xFFFF0000 : val;
};


// buff.readInt32LE(offset[,noAssert])
// [1] buff.readInt32LE(offset)
// [2] buff.readInt32LE(offset, noAssert)
Buffer.prototype.readInt32LE = function(offset, noAssert) {
  offset = offset >>> 0;
  if (!noAssert)
    checkOffset(offset, 4, this.length);
  return this._builtin.readUInt8(offset) |
         (this._builtin.readUInt8(offset + 1) << 8) |
         (this._builtin.readUInt8(offset + 2) << 16) |
         (this._builtin.readUInt8(offset + 3) << 24);
};

// buff.readInt32BE(offset[,noAssert])
// [1] buff.readInt32BE(offset)
// [2] buff.readInt32BE(offset, noAssert)
Buffer.prototype.readInt32BE = function(offset, noAssert) {
  offset = offset >>> 0;
  if (!noAssert)
    checkOffset(offset, 4, this.length);
  return this._builtin.readUInt8(offset + 3) |
         (this._builtin.readUInt8(offset + 2) << 8) |
         (this._builtin.readUInt8(offset + 1) << 16) |
         (this._builtin.readUInt8(offset) << 24);
};

// buff.readFloatLE(offset[,noAssert])
// [1] buff.readFloatLE(offset)
// [2] buff.readFloatLE(offset, noAssert)
Buffer.prototype.readFloatLE = function(offset, noAssert) {
  offset = offset >>> 0;
  if (!noAssert)
    checkOffset(offset, 4, this.length);
  return this._builtin.readFloatLE(offset);
};


// buff.readDoubleLE(offset[,noAssert])
// [1] buff.readDoubleLE(offset)
// [2] buff.readDoubleLE(offset, noAssert)
Buffer.prototype.readDoubleLE = function(offset, noAssert) {
  offset = offset >>> 0;
  if (!noAssert)
    checkOffset(offset, 8, this.length);
  return this._builtin.readDoubleLE(offset);
};


// buff.fill(value)
Buffer.prototype.fill = function(value) {
  if (util.isNumber(value)) {
    value = value & 255;
    for (var i = 0; i < this.length; i++) {
      this._builtin.writeUInt8(value, i);
    }
  }
  return this;
};


Buffer.prototype.valid = function(encoding) {
  encoding = encoding || 'utf8';
  if (encoding === 'utf8') {
    return this._builtin._isUtf8String();
  } else {
    throw new TypeError(`Unknown ${encoding} validation.`);
  }
};


Buffer.from = function(arg, encodingOrOffset, length) {
  if (typeof arg === 'number') {
    throw new TypeError('Argument must not be a number');
  }
  return new Buffer(arg, encodingOrOffset, length);
};

Buffer.alloc = function(size, fill, encoding) {
  if (typeof size !== 'number') {
    throw new TypeError('Argument must be a number');
  }
  var buf = new Buffer(size);
  if (fill !== undefined) {
    if (typeof encoding === 'string') {
      buf.fill(fill, encoding);
    } else {
      buf.fill(fill);
    }
  } else {
    buf.fill(0);
  }
  return buf;
};

Buffer.allocUnsafe = function(size) {
  if (typeof size !== 'number') {
    throw new TypeError('Argument must be a number');
  }
  return new Buffer(size);
};

Buffer.allocUnsafeSlow = function(size) {
  if (typeof size !== 'number') {
    throw new TypeError('Argument must be a number');
  }
  return new Buffer(size);
};


Object.defineProperty(Buffer.prototype, 'byteLength', {
  get: function() {
    return this.length;
  },
  set: function() {
    throw new Error('cannot set Buffer.prototype.byteLength');
  }
});

module.exports = Buffer;
module.exports.Buffer = Buffer;
