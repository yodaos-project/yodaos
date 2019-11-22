'use strict';

// var _randomBytes = native.randomBytes;
var _randomBytesSync = native.randomBytesSync;
var _Hash = require('crypto_hash').Hash;

function Hash(algorithm) {
  if (!Hash._hashes[algorithm]) {
    throw new Error('Unknown hash algorithm ' + algorithm);
  }
  this._handle = new _Hash(Hash._hashes[algorithm]);
}

Hash.prototype.update = function(buf, inputEncoding) {
  if (typeof buf !== 'string' && !Buffer.isBuffer(buf)) {
    throw new TypeError(
      'Expect buffer or string on first argument of cipher.update.');
  }
  if (typeof buf === 'string') {
    buf = Buffer.from(buf, inputEncoding);
  }
  this._handle.update(buf);
  return this;
};

Hash.prototype.digest = function(encoding) {
  var buf = this._handle.digest();
  if (typeof encoding === 'string') {
    return buf.toString(encoding);
  } else {
    return buf;
  }
};

Hash._hashes = {
  'none': 0,
  'md2': 1,
  'md4': 2,
  'md5': 3,
  'sha1': 4,
  'sha224': 5,
  'sha256': 6,
  'sha384': 7,
  'sha512': 8,
  'ripemd160': 9
};

exports.createHash = function(algorithm) {
  return new Hash(algorithm);
};

exports.getHashes = function() {
  return Object.keys(Hash._hashes);
};

exports.randomBytes = function(size, callback) {
  var bytes = _randomBytesSync(size);
  var buf = new Buffer(bytes);
  if (typeof callback === 'function') {
    callback(null, buf);
  }
  return buf;
};
