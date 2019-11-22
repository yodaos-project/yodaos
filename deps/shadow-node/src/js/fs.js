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

var fs = exports;
var constants = require('constants');
var url = require('url');
var util = require('util');
var stream = require('stream');

var fsBuiltin = native;
var Readable = stream.Readable;
var Writable = stream.Writable;

// constants
var kMinPoolSpace = 128;

function assertEncoding(encoding) {
  // TODO
}

function getOptions(options, defaultOptions) {
  if (options === null || options === undefined ||
      typeof options === 'function') {
    return defaultOptions;
  }

  if (typeof options === 'string') {
    defaultOptions = Object.assign({}, defaultOptions);
    defaultOptions.encoding = options;
    options = defaultOptions;
  } else if (typeof options !== 'object') {
    throw new TypeError('ERR_INVALID_ARG_TYPE');
  }

  if (options.encoding !== 'buffer')
    assertEncoding(options.encoding);
  return options;
}

function copyObject(source) {
  var target = {};
  for (var key in source)
    target[key] = source[key];
  return target;
}

function handleError(val, callback) {
  if (val instanceof Error) {
    if (typeof callback === 'function') {
      process.nextTick(callback, val);
      return true;
    } else throw val;
  }
  return false;
}

var pool;
function allocNewPool(poolSize) {
  // TODO(Yorkie): implement Buffer.allocUnsafe to replace
  pool = new Buffer(poolSize);
  pool.used = 0;
}

function getPathFromURL(path) {
  if (path == null || !path.query) {
    return path;
  }
  if (path.protocol !== 'file:') {
    return new TypeError('ERR_INVALID_URL_SCHEME');
  }

  if (url.hostname !== '') {
    return new TypeError('ERR_INVALID_FILE_URL_HOST');
  }
  var pathname = url.pathname;
  for (var n = 0; n < pathname.length; n++) {
    if (pathname[n] === '%') {
      var third = pathname.codePointAt(n + 2) | 0x20;
      if (pathname[n + 1] === '2' && third === 102) {
        return new TypeError('ERR_INVALID_FILE_URL_PATH');
      }
    }
  }
  return decodeURIComponent(pathname);
}

fs.exists = function(path, callback) {
  if (!path || !path.length) {
    process.nextTick(function() {
      if (callback) callback(false);
    });
    return;
  }

  function cb(err, stat) {
    if (callback) callback(err ? false : true);
  }

  fsBuiltin.stat(checkArgString(path, 'path'),
                 checkArgFunction(cb, 'callback'));
};


fs.existsSync = function(path) {
  if (!path || !path.length) {
    return false;
  }

  try {
    fsBuiltin.stat(checkArgString(path, 'path'));
    return true;
  } catch (e) {
    return false;
  }
};


function _checkModeProperty(mode, property) {
  return ((mode & constants.S_IFMT) === property);
}


function addXTimeProerties(stat) {
  stat.atime = new Date(stat.atimeMs);
  stat.mtime = new Date(stat.mtimeMs);
  stat.ctime = new Date(stat.ctimeMs);
  stat.birthtime = new Date(stat.birthtimeMs);
  stat.isDirectory = function() {
    return _checkModeProperty(stat.mode, constants.S_IFDIR);
  };
  stat.isFile = function() {
    return _checkModeProperty(stat.mode, constants.S_IFREG);
  };
  stat.isSymbolicLink = function() {
    return _checkModeProperty(stat.mode, constants.S_IFLNK);
  };
  stat.isFIFO = function() {
    return _checkModeProperty(stat.mode, constants.S_IFIFO);
  };
  stat.isSocket = function() {
    return _checkModeProperty(stat.mode, constants.S_IFSOCK);
  };
  return stat;
}

fs.stat = function(path, callback) {
  var cb = checkArgFunction(callback, 'callback');
  fsBuiltin.stat(checkArgString(path, 'path'), function(err, stat) {
    if (err) return cb(err);
    return cb(null, addXTimeProerties(stat));
  });
};


fs.statSync = function(path) {
  var val = fsBuiltin.stat(checkArgString(path, 'path'));
  return addXTimeProerties(val);
};


fs.lstat = function(path, callback) {
  var cb = checkArgFunction(callback, 'callback');
  fsBuiltin.lstat(checkArgString(path, 'path'), function(err, stat) {
    if (err) return cb(err);
    return cb(null, addXTimeProerties(stat));
  });
};


fs.lstatSync = function(path) {
  var val = fsBuiltin.lstat(checkArgString(path, 'path'));
  return addXTimeProerties(val);
};


fs.fstat = function(fd, callback) {
  fsBuiltin.fstat(checkArgNumber(fd, 'fd'),
                  checkArgFunction(callback, 'callback'));
};


fs.fstatSync = function(fd) {
  return fsBuiltin.fstat(checkArgNumber(fd, 'fd'));
};


fs.close = function(fd, callback) {
  fsBuiltin.close(checkArgNumber(fd, 'fd'),
                  checkArgFunction(callback, 'callback'));
};


fs.closeSync = function(fd) {
  fsBuiltin.close(checkArgNumber(fd, 'fd'));
};


fs.open = function(path, flags, mode, callback) {
  fsBuiltin.open(checkArgString(path, 'path'),
                 convertFlags(flags),
                 convertMode(mode, 438),
                 checkArgFunction(arguments[arguments.length - 1]), 'callback');
};


fs.openSync = function(path, flags, mode) {
  return fsBuiltin.open(checkArgString(path, 'path'),
                        convertFlags(flags),
                        convertMode(mode, 438));
};


fs.read = function(fd, buffer, offset, length, position, callback) {
  if (util.isNullOrUndefined(position)) {
    position = -1; // Read from the current position.
  }

  callback = checkArgFunction(callback, 'callback');

  function cb(err, bytesRead) {
    callback(err, bytesRead || 0, buffer);
  }

  return fsBuiltin.read(checkArgNumber(fd, 'fd'),
                        checkArgBuffer(buffer, 'buffer'),
                        checkArgNumber(offset, 'offset'),
                        checkArgNumber(length, 'length'),
                        checkArgNumber(position, 'position'),
                        cb);
};


fs.readSync = function(fd, buffer, offset, length, position) {
  if (util.isNullOrUndefined(position)) {
    position = -1; // Read from the current position.
  }

  return fsBuiltin.read(checkArgNumber(fd, 'fd'),
                        checkArgBuffer(buffer, 'buffer'),
                        checkArgNumber(offset, 'offset'),
                        checkArgNumber(length, 'length'),
                        checkArgNumber(position, 'position'));
};


fs.write = function(fd, buffer, offset, length, position, callback) {
  if (util.isFunction(position)) {
    callback = position;
    position = -1; // write at current position.
  } else if (util.isNullOrUndefined(position)) {
    position = -1; // write at current position.
  }

  callback = checkArgFunction(callback, 'callback');

  function cb(err, written) {
    callback(err, written, buffer);
  }

  return fsBuiltin.write(checkArgNumber(fd, 'fd'),
                         checkArgBuffer(buffer, 'buffer'),
                         checkArgNumber(offset, 'offset'),
                         checkArgNumber(length, 'length'),
                         checkArgNumber(position, 'position'),
                         cb);
};


fs.writeSync = function(fd, buffer, offset, length, position) {
  if (util.isNullOrUndefined(position)) {
    position = -1; // write at current position.
  }

  return fsBuiltin.write(checkArgNumber(fd, 'fd'),
                         checkArgBuffer(buffer, 'buffer'),
                         checkArgNumber(offset, 'offset'),
                         checkArgNumber(length, 'length'),
                         checkArgNumber(position, 'position'));
};


fs.readFile = function(path, options, callback) {
  if (util.isFunction(options)) {
    callback = options;
    options = null;
  }
  checkArgString(path);
  checkArgFunction(callback);

  var fd;
  var buffers;

  fs.open(path, 'r', function(err, _fd) {
    if (err) {
      return callback(err);
    }

    fd = _fd;
    buffers = [];

    // start read
    read();
  });

  function read() {
    // Read segment of data.
    var buffer = new Buffer(1023);
    fs.read(fd, buffer, 0, 1023, -1, afterRead);
  }

  function afterRead(err, bytesRead, buffer) {
    if (err) {
      fs.close(fd, function(err) {
        return callback(err);
      });
    }

    if (bytesRead === 0) {
      // End of file.
      close();
    } else {
      // continue reading.
      buffers.push(buffer.slice(0, bytesRead));
      read();
    }
  }

  function close() {
    fs.close(fd, function(err) {
      var buf = Buffer.concat(buffers);
      if (options) {
        var enc;
        if (util.isString(options))
          enc = options;
        if (options.encoding && util.isString(options.encoding))
          enc = options && options.encoding;
        if (enc === 'utf8' || enc === 'utf-8' || enc === 'hex') {
          buf = buf.toString(enc);
        }
      }
      return callback(err, buf);
    });
  }
};


fs.readFileSync = function(path, encoding) {
  checkArgString(path);

  var fd = fs.openSync(path, 'r', 438);
  var buffers = [];

  while (true) {
    try {
      var buffer = new Buffer(1023);
      var bytesRead = fs.readSync(fd, buffer, 0, 1023);
      if (bytesRead) {
        buffers.push(buffer.slice(0, bytesRead));
      } else {
        break;
      }
    } catch (e) {
      break;
    }
  }
  fs.closeSync(fd);

  var ret = Buffer.concat(buffers);
  if (encoding) {
    ret = ret.toString(encoding);
  }
  return ret;
};


fs.writeFile = function(path, data, options, callback) {
  if (util.isFunction(options))
    callback = options;

  checkArgString(path);
  checkArgFunction(callback);

  var fd;
  var len;
  var bytesWritten;
  var buffer = ensureBuffer(data);

  fs.open(path, 'w', function(err, _fd) {
    if (err) {
      return callback(err);
    }

    fd = _fd;
    len = buffer.length;
    bytesWritten = 0;

    write();
  });

  function write() {
    var tryN = (len - bytesWritten) >= 1024 ? 1023 : (len - bytesWritten);
    fs.write(fd, buffer, bytesWritten, tryN, bytesWritten, afterWrite);
  }

  function afterWrite(err, n) {
    if (err) {
      fs.close(fd, function(err) {
        return callback(err);
      });
    }

    if (n <= 0 || bytesWritten + n === len) {
      // End of buffer
      fs.close(fd, function(err) {
        callback(err);
      });
    } else {
      // continue writing
      bytesWritten += n;
      write();
    }
  }
};


fs.writeFileSync = function(path, data) {
  checkArgString(path);

  var buffer = ensureBuffer(data);
  var fd = fs.openSync(path, 'w');
  var len = buffer.length;
  var bytesWritten = 0;

  while (true) {
    try {
      var tryN = (len - bytesWritten) >= 1024 ? 1023 : (len - bytesWritten);
      var n = fs.writeSync(fd, buffer, bytesWritten, tryN, bytesWritten);
      bytesWritten += n;
      if (bytesWritten === len) {
        break;
      }
    } catch (e) {
      break;
    }
  }
  fs.closeSync(fd);
  return bytesWritten;
};


fs.mkdir = function(path, mode, callback) {
  if (util.isFunction(mode)) callback = mode;
  checkArgString(path, 'path');
  checkArgFunction(callback, 'callback');
  fsBuiltin.mkdir(path, convertMode(mode, 511), callback);
};


fs.mkdirSync = function(path, mode) {
  return fsBuiltin.mkdir(checkArgString(path, 'path'),
                         convertMode(mode, 511));
};


fs.rmdir = function(path, callback) {
  checkArgString(path, 'path');
  checkArgFunction(callback, 'callback');
  fsBuiltin.rmdir(path, callback);
};


fs.rmdirSync = function(path) {
  return fsBuiltin.rmdir(checkArgString(path, 'path'));
};


fs.unlink = function(path, callback) {
  checkArgString(path);
  checkArgFunction(callback);
  fsBuiltin.unlink(path, callback);
};


fs.unlinkSync = function(path) {
  return fsBuiltin.unlink(checkArgString(path, 'path'));
};


fs.rename = function(oldPath, newPath, callback) {
  checkArgString(oldPath);
  checkArgString(newPath);
  checkArgFunction(callback);
  fsBuiltin.rename(oldPath, newPath, callback);
};


fs.renameSync = function(oldPath, newPath) {
  checkArgString(oldPath);
  checkArgString(newPath);
  fsBuiltin.rename(oldPath, newPath);
};


fs.readdir = function(path, callback) {
  checkArgString(path);
  checkArgFunction(callback);
  fsBuiltin.readdir(path, callback);
};


fs.readdirSync = function(path) {
  return fsBuiltin.readdir(checkArgString(path, 'path'));
};


function convertFlags(flag) {
  var O_APPEND = constants.O_APPEND;
  var O_CREAT = constants.O_CREAT;
  var O_EXCL = constants.O_EXCL;
  var O_RDONLY = constants.O_RDONLY;
  var O_RDWR = constants.O_RDWR;
  var O_SYNC = constants.O_SYNC;
  var O_TRUNC = constants.O_TRUNC;
  var O_WRONLY = constants.O_WRONLY;

  if (util.isString(flag)) {
    switch (flag) {
      case 'r': return O_RDONLY;
      case 'rs':
      case 'sr': return O_RDONLY | O_SYNC;

      case 'r+': return O_RDWR;
      case 'rs+':
      case 'sr+': return O_RDWR | O_SYNC;

      case 'w': return O_TRUNC | O_CREAT | O_WRONLY;
      case 'wx':
      case 'xw': return O_TRUNC | O_CREAT | O_WRONLY | O_EXCL;

      case 'w+': return O_TRUNC | O_CREAT | O_RDWR;
      case 'wx+':
      case 'xw+': return O_TRUNC | O_CREAT | O_RDWR | O_EXCL;

      case 'a': return O_APPEND | O_CREAT | O_WRONLY;
      case 'ax':
      case 'xa': return O_APPEND | O_CREAT | O_WRONLY | O_EXCL;

      case 'a+': return O_APPEND | O_CREAT | O_RDWR;
      case 'ax+':
      case 'xa+': return O_APPEND | O_CREAT | O_RDWR | O_EXCL;
    }
  }
  throw new TypeError('Bad argument: flags');
}


function convertMode(mode, def) {
  if (util.isNumber(mode)) {
    return mode;
  } else if (util.isString(mode)) {
    return parseInt(mode, 8);
  } else if (def) {
    return convertMode(def);
  }
  return undefined;
}


function ensureBuffer(data) {
  if (util.isBuffer(data)) {
    return data;
  }

  return new Buffer(data + ''); // coert to string and make it a buffer
}


function checkArgType(value, name, checkFunc) {
  if (checkFunc(value)) {
    return value;
  } else {
    throw new TypeError('Bad arguments: ' + name);
  }
}


function checkArgBuffer(value, name) {
  return checkArgType(value, name, util.isBuffer);
}


function checkArgNumber(value, name) {
  return checkArgType(value, name, util.isNumber);
}


function checkArgString(value, name) {
  return checkArgType(value, name, util.isString);
}


function checkArgFunction(value, name) {
  return checkArgType(value, name, util.isFunction);
}

// ReadStream
fs.createReadStream = function(path, options) {
  return new ReadStream(path, options);
};

util.inherits(ReadStream, Readable);
fs.ReadStream = ReadStream;
fs.FileReadStream = ReadStream;

function ReadStream(path, options) {
  if (!(this instanceof ReadStream))
    return new ReadStream(path, options);

  // a little bit bigger buffer and water marks by default
  options = copyObject(getOptions(options, {}));
  if (options.highWaterMark === undefined)
    options.highWaterMark = 64 * 1024;

  Readable.call(this, options);

  handleError((this.path = getPathFromURL(path)));
  this.fd = options.fd === undefined ? null : options.fd;
  this.flags = options.flags === undefined ? 'r' : options.flags;
  this.mode = options.mode === undefined ? 438 /* 0o666 */ : options.mode;

  this.start = typeof this.fd !== 'number' && options.start === undefined ?
    0 : options.start;
  this.end = options.end;
  this.autoClose = options.autoClose === undefined ? true : options.autoClose;
  this.pos = undefined;
  this.bytesRead = 0;
  this.closed = false;

  if (this.start !== undefined) {
    if (typeof this.start !== 'number') {
      throw new TypeError('ERR_INVALID_ARG_TYPE');
    }
    if (this.end === undefined) {
      this.end = Infinity;
    } else if (typeof this.end !== 'number') {
      throw new TypeError('ERR_INVALID_ARG_TYPE');
    }
    if (this.start > this.end) {
      throw new RangeError('ERR_OUT_OF_RANGE');
    }
    this.pos = this.start;
  }

  if (typeof this.fd !== 'number')
    this.open();

  this.on('end', function() {
    if (this.autoClose) {
      this._destroy();
    }
  }.bind(this));
}

ReadStream.prototype.open = function() {
  var self = this;
  fs.open(this.path, this.flags, this.mode, function(er, fd) {
    if (er) {
      if (self.autoClose) {
        self._destroy();
      }
      self.emit('error', er);
      return;
    }

    self.fd = fd;
    self.emit('open', fd);
    // start the flow of data.
    self._read();
  });
};

ReadStream.prototype._read = function(n) {
  if (this.destroyed)
    return;

  if (!pool || pool.length - pool.used < kMinPoolSpace) {
    // discard the old pool.
    allocNewPool(this.readableHighWaterMark);
  }

  // Grab another reference to the pool in the case that while we're
  // in the thread pool another read() finishes up the pool, and
  // allocates a new one.
  var thisPool = pool;
  var toRead = Math.min(
    pool.length - pool.used, n || this.readableHighWaterMark);
  var start = pool.used;

  if (this.pos !== undefined)
    toRead = Math.min(this.end - this.pos + 1, toRead);

  // already read everything we were supposed to read!
  // treat as EOF.
  if (toRead <= 0)
    return this.push(null);

  // the actual read.
  fs.read(this.fd, pool, pool.used, toRead, this.pos, function(er, bytesRead) {
    if (er) {
      if (this.autoClose) {
        this.destroy();
      }
      this.emit('error', er);
    } else {
      var b = null;
      if (bytesRead > 0) {
        this.bytesRead += bytesRead;
        b = thisPool.slice(start, start + bytesRead);
      }
      this.push(b);

      // check if read is less than 0, try read again.
      if (bytesRead > 0) {
        this._read();
      }
    }
  }.bind(this));

  // move the pool positions, and internal position for reading.
  if (this.pos !== undefined)
    this.pos += toRead;
  pool.used += toRead;
};

ReadStream.prototype._destroy = function(err, cb) {
  var isOpen = typeof this.fd !== 'number';
  if (isOpen) {
    this.once('open', closeFsStream.bind(null, this, cb, err));
    return;
  }

  closeFsStream(this, cb);
  this.fd = null;
};

function closeFsStream(stream, cb, err) {
  fs.close(stream.fd, function(er) {
    er = er || err;
    if (typeof cb === 'function')
      cb(er);
    stream.closed = true;
    if (!er)
      stream.emit('close');
  });
}

ReadStream.prototype.close = function(cb) {
  this._destroy(null, cb);
};

// Write Stream
fs.createWriteStream = function(path, options) {
  return new WriteStream(path, options);
};

util.inherits(WriteStream, Writable);
fs.WriteStream = WriteStream;
fs.FileWriteStream = WriteStream;

function WriteStream(path, options) {
  if (!(this instanceof WriteStream))
    return new WriteStream(path, options);

  options = copyObject(getOptions(options, {}));

  Writable.call(this, options);

  handleError((this.path = getPathFromURL(path)));
  this.fd = options.fd === undefined ? null : options.fd;
  this.flags = options.flags === undefined ? 'w' : options.flags;
  this.mode = options.mode === undefined ? 438 /* 0o666 */ : options.mode;

  this.start = options.start;
  this.autoClose = options.autoClose === undefined ? true : !!options.autoClose;
  this.pos = undefined;
  this.bytesWritten = 0;
  this.closed = false;

  if (this.start !== undefined) {
    if (typeof this.start !== 'number') {
      throw new TypeError('ERR_INVALID_ARG_TYPE');
    }
    if (this.start < 0) {
      throw new RangeError('ERR_OUT_OF_RANGE');
    }
    this.pos = this.start;
  }

  if (options.encoding)
    this.setDefaultEncoding(options.encoding);

  if (typeof this.fd !== 'number')
    this.open();

  // dispose on finish.
  this.once('finish', function() {
    if (this.autoClose) {
      this._destroy();
    }
  });
}

WriteStream.prototype.open = function() {
  fs.open(this.path, this.flags, this.mode, function(er, fd) {
    if (er) {
      if (this.autoClose) {
        this._destroy();
      }
      this.emit('error', er);
      return;
    }

    this.fd = fd;
    this.emit('open', fd);
    this._readyToWrite();
  }.bind(this));
};


WriteStream.prototype._write = function(data, encoding, cb, onwrite) {
  if (!(data instanceof Buffer)) {
    var err = new TypeError('ERR_INVALID_ARG_TYPE');
    return this.emit('error', err);
  }

  if (typeof this.fd !== 'number') {
    return this.once('open', function() {
      this._write(data, encoding, cb, onwrite);
    });
  }

  fs.write(this.fd, data, 0, data.length, this.pos, function(er, bytes) {
    if (er) {
      if (this.autoClose) {
        this._destroy();
      }
      process.nextTick(onwrite);
      if (util.isFunction(cb)) {
        process.nextTick(function() {
          cb(er);
        });
      }
      return;
    }
    this.bytesWritten += bytes;
    onwrite();
    cb();
  }.bind(this));

  if (this.pos !== undefined)
    this.pos += data.length;
};

WriteStream.prototype._destroy = ReadStream.prototype._destroy;
WriteStream.prototype.close = function(cb) {
  if (cb) {
    if (this.closed) {
      process.nextTick(cb);
      return;
    } else {
      this.on('close', cb);
    }
  }

  // If we are not autoClosing, we should call
  // destroy on 'finish'.
  if (!this.autoClose) {
    this.on('finish', this.destroy.bind(this));
  }

  // we use end() instead of destroy() because of
  // https://github.com/nodejs/node/issues/2006
  this.end();
};

// There is no shutdown() for files.
WriteStream.prototype.destroySoon = WriteStream.prototype.end;
