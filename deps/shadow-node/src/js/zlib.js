'use strict';

var Transform = require('stream').Transform;
var util = require('util');
var constants = native;
var zlib_modes = {
  none: 0,
  deflate: 1,
  inflate: 2,
  gzip: 3,
  gunzip: 4,
  deflateraw: 5,
  inflateraw: 6,
  unzip: 7,
};

// translation table for return codes.
var codes = {
  Z_OK: constants.Z_OK,
  Z_STREAM_END: constants.Z_STREAM_END,
  Z_NEED_DICT: constants.Z_NEED_DICT,
  Z_ERRNO: constants.Z_ERRNO,
  Z_STREAM_ERROR: constants.Z_STREAM_ERROR,
  Z_DATA_ERROR: constants.Z_DATA_ERROR,
  Z_MEM_ERROR: constants.Z_MEM_ERROR,
  Z_BUF_ERROR: constants.Z_BUF_ERROR,
  Z_VERSION_ERROR: constants.Z_VERSION_ERROR,
};

function zlibOnError(message, errno) {
  var self = this.jsref;
  // there is no way to cleanly recover.
  // continuing only obscures problems.
  _close(self);
  self._hadError = true;

  var error = new Error(message);
  error.errno = errno;
  error.code = codes[errno];
  self.emit('error', error);
}

function Zlib(opts, mode) {
  var chunkSize = constants.Z_DEFAULT_CHUNK;
  var flush = constants.Z_NO_FLUSH;
  var finishFlush = constants.Z_FINISH;
  var windowBits = constants.Z_DEFAULT_WINDOWBITS;
  var level = constants.Z_DEFAULT_COMPRESSION;
  var memLevel = constants.Z_DEFAULT_MEMLEVEL;
  var strategy = constants.Z_DEFAULT_STRATEGY;
  var dictionary;

  if (typeof mode !== 'number')
    throw new TypeError('ERR_INVALID_ARG_TYPE');
  if (mode < constants.DEFLATE || mode > constants.UNZIP)
    throw new RangeError('ERR_OUT_OF_RANGE');

  Transform.call(this, opts);
  this.bytesRead = 0;
  this._handle = new native.Zlib(mode);
  this._handle.jsref = this; // Used by processCallback() and zlibOnError()
  this._handle.onerror = zlibOnError;
  this._hadError = false;
  this._writeState = new Array(2);

  if (!this._handle.init(windowBits,
                         level,
                         memLevel,
                         strategy,
                         this._writeState,
                         processCallback,
                         dictionary)) {
    throw new Error('ERR_ZLIB_INITIALIZATION_FAILED');
  }

  this._outBuffer = new Buffer(chunkSize);
  this._outOffset = 0;
  this._level = level;
  this._strategy = strategy;
  this._chunkSize = chunkSize;
  this._flushFlag = flush;
  this._scheduledFlushFlag = constants.Z_NO_FLUSH;
  this._origFlushFlag = flush;
  this._finishFlushFlag = finishFlush;
  this._info = opts && opts.info;
  this.once('end', this.close);
}
util.inherits(Zlib, Transform);

Object.defineProperty(Zlib.prototype, '_closed', {
  configurable: true,
  enumerable: true,
  get: function() {
    return !this._handle;
  }
});

function processCallback() {
  // This callback's context (`this`) is the `_handle` (ZCtx) object. It is
  // important to null out the values once they are no longer needed since
  // `_handle` can stay in memory long after the buffer is needed.
  var handle = this;
  var self = this.jsref;
  var state = self._writeState;

  if (self._hadError) {
    this.buffer = null;
    return;
  }

  if (self.destroyed) {
    this.buffer = null;
    return;
  }

  var availOutAfter = state[0];
  var availInAfter = state[1];

  var inDelta = (handle.availInBefore - availInAfter);
  self.bytesRead += inDelta;

  var have = handle.availOutBefore - availOutAfter;
  if (have > 0) {
    var out = self._outBuffer.slice(self._outOffset, self._outOffset + have);
    self._outOffset += have;
    self.push(out);
  } else if (have < 0) {
    throw new Error('have should not go down');
  }

  // exhausted the output buffer, or used all the input create a new one.
  if (availOutAfter === 0 || self._outOffset >= self._chunkSize) {
    handle.availOutBefore = self._chunkSize;
    self._outOffset = 0;
    self._outBuffer = new Buffer(self._chunkSize);
  }

  if (availOutAfter === 0) {
    // Not actually done. Need to reprocess.
    // Also, update the availInBefore to the availInAfter value,
    // so that if we have to hit it a third (fourth, etc.) time,
    // it'll have the correct byte counts.
    handle.inOff += inDelta;
    handle.availInBefore = availInAfter;

    this.write(handle.flushFlag,
               this.buffer, // in
               handle.inOff, // in_off
               handle.availInBefore, // in_len
               self._outBuffer, // out
               self._outOffset, // out_off
               self._chunkSize); // out_len
    this._doWrite();
    return;
  }

  // finished with the chunk.
  this.buffer = null;
  this.cb();
}

function _close(engine, callback) {
  if (callback)
    process.nextTick(callback);

  // Caller may invoke .close after a zlib error (which will null _handle).
  if (!engine._handle)
    return;

  engine._handle.close();
  engine._handle = null;
}

function emitCloseNT(self) {
  self.emit('close');
}

Zlib.prototype.close = function close(callback) {
  _close(this, callback);
  process.nextTick(emitCloseNT, this);
};

Zlib.prototype.reset = function() {
  if (!this._handle)
    throw new Error('zlib binding closed');
  return this._handle.reset();
};

Zlib.prototype._flush = function _flush(callback) {
  this._transform(new Buffer(0), '', callback);
};

Zlib.prototype._transform = function _transform(chunk, encoding, cb) {
  // If it's the last chunk, or a final flush, we use the Z_FINISH flush flag
  // (or whatever flag was provided using opts.finishFlush).
  // If it's explicitly flushing at some other time, then we use
  // Z_FULL_FLUSH. Otherwise, use the original opts.flush flag.
  var flushFlag;
  var ws = this._writableState;
  if ((ws.ending || ws.ended) && ws.length === chunk.byteLength) {
    flushFlag = this._finishFlushFlag;
  } else {
    flushFlag = this._flushFlag;
    // once we've flushed the last of the queue, stop flushing and
    // go back to the normal behavior.
    if (chunk.byteLength >= ws.length)
      this._flushFlag = this._origFlushFlag;
  }
  processChunk(this, chunk, flushFlag, cb);
};

function processChunk(self, chunk, flushFlag, cb) {
  var handle = self._handle;
  if (!handle)
    return cb(new Error('ERR_ZLIB_BINDING_CLOSED'));

  handle.buffer = chunk;
  handle.cb = cb;
  handle.availOutBefore = self._chunkSize - self._outOffset;
  handle.availInBefore = chunk.byteLength;
  handle.inOff = 0;
  handle.flushFlag = flushFlag;

  handle.write(flushFlag,
               chunk, // in
               0, // in_off
               handle.availInBefore, // in_len
               self._outBuffer, // out
               self._outOffset, // out_off
               handle.availOutBefore); // out_len
  handle._doWrite();
}

// generic zlib
// minimal 2-byte header
function Deflate(opts) {
  if (!(this instanceof Deflate))
    return new Deflate(opts);
  Zlib.call(this, opts, zlib_modes.deflate);
}
util.inherits(Deflate, Zlib);

function Inflate(opts) {
  if (!(this instanceof Inflate))
    return new Inflate(opts);
  Zlib.call(this, opts, zlib_modes.inflate);
}
util.inherits(Inflate, Zlib);

function Gzip(opts) {
  if (!(this instanceof Gzip))
    return new Gzip(opts);
  Zlib.call(this, opts, zlib_modes.gzip);
}
util.inherits(Gzip, Zlib);

function Gunzip(opts) {
  if (!(this instanceof Gunzip))
    return new Gunzip(opts);
  Zlib.call(this, opts, zlib_modes.gunzip);
}
util.inherits(Gunzip, Zlib);

function DeflateRaw(opts) {
  if (opts && opts.windowBits === 8) opts.windowBits = 9;
  if (!(this instanceof DeflateRaw))
    return new DeflateRaw(opts);
  Zlib.call(this, opts, zlib_modes.deflateraw);
}
util.inherits(DeflateRaw, Zlib);

function InflateRaw(opts) {
  if (!(this instanceof InflateRaw))
    return new InflateRaw(opts);
  Zlib.call(this, opts, zlib_modes.inflateraw);
}
util.inherits(InflateRaw, Zlib);

function Unzip(opts) {
  if (!(this instanceof Unzip))
    return new Unzip(opts);
  Zlib.call(this, opts, zlib_modes.unzip);
}
util.inherits(Unzip, Zlib);

function createProperty(ctor) {
  return {
    configurable: true,
    enumerable: true,
    value: function(options) {
      return new ctor(options);
    }
  };
}

module.exports = {
  Deflate: Deflate,
  Inflate: Inflate,
  Gzip: Gzip,
  Gunzip: Gunzip,
  DeflateRaw: DeflateRaw,
  InflateRaw: InflateRaw,
  Unzip: Unzip,
};

Object.defineProperties(module.exports, {
  createDeflate: createProperty(Deflate),
  createInflate: createProperty(Inflate),
  createDeflateRaw: createProperty(DeflateRaw),
  createInflateRaw: createProperty(InflateRaw),
  createGzip: createProperty(Gzip),
  createGunzip: createProperty(Gunzip),
  createUnzip: createProperty(Unzip),
  codes: {
    enumerable: true,
    writable: false,
    value: Object.freeze(codes)
  }
});
