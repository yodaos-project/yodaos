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
var stream = require('stream');

var crlfBuf = Buffer.from('\r\n');


function OutgoingMessage() {
  stream.Stream.call(this);

  this.writable = true;

  this._hasBody = true;
  this._contentLength = null;
  this.chunkedEncoding = false;
  this.useChunkedEncodingByDefault = true;

  this.finished = false;
  this._sentHeader = false;

  this.socket = null;
  this.connection = null;
  // response header string : same 'content' as this._headers
  this._header = null;
  // response header obj : (key, value) pairs
  this._headers = {};

  // if user explicitly remove headers
  this._removedTransferEncoding = false;
  this._removedContentLength = false;
}

util.inherits(OutgoingMessage, stream.Stream);

exports.OutgoingMessage = OutgoingMessage;


OutgoingMessage.prototype.end = function(data, encoding, callback) {
  var self = this;

  if (util.isFunction(data)) {
    callback = data;
    data = null;
  } else if (util.isFunction(encoding)) {
    callback = encoding;
    encoding = null;
  }

  if (this.finished) {
    return false;
  }

  if (data) {
    if (!this._header) {
      if (typeof data === 'string') {
        this._contentLength = Buffer.byteLength(data, encoding);
      } else {
        this._contentLength = data.length;
      }
    }
    this.write(data, encoding);
  } else if (!this._header) {
    this._contentLength = 0;
    this._implicitHeader();
  }

  if (this.chunkedEncoding) {
    this.write('');
  }

  // Register finish event handler.
  if (util.isFunction(callback)) {
    this.once('finish', callback);
  }

  // Force flush buffered data.
  // After all data was sent, emit 'finish' event meaning segment of header and
  // body were all sent finished. This means different from 'finish' event
  // emitted by net which indicate there will be no more data to be sent through
  // the connection. On the other hand emitting 'finish' event from http does
  // not neccessarily imply end of data transmission since there might be
  // another segment of data when connection is 'Keep-Alive'.
  this._send('', function() {
    self.emit('finish');
  });


  this.finished = true;

  this._finish();

  return true;
};


OutgoingMessage.prototype._finish = function() {
  this.emit('prefinish');
};


// This sends chunk directly into socket
// TODO: buffering of chunk in the case of socket is not available
OutgoingMessage.prototype._send = function(chunk, encoding, callback) {
  if (util.isFunction(encoding)) {
    callback = encoding;
  }

  if (util.isBuffer(chunk)) {
    chunk = chunk.toString();
  }
  if (!this._sentHeader) {
    chunk = this._header + '\r\n' + chunk;
    this._sentHeader = true;
  }

  return this.connection.write(chunk, encoding, callback);
};


OutgoingMessage.prototype.write = function(chunk, encoding, callback) {
  if (!this._header) {
    this._implicitHeader();
  }

  if (!this._hasBody) {
    return true;
  }

  var len, ret;
  if (this.chunkedEncoding) {
    if (typeof chunk === 'string') {
      len = Buffer.byteLength(chunk, encoding);
    } else {
      len = chunk.length;
    }

    this._send(len.toString(16), 'latin1', null);
    this._send(crlfBuf, null, null);
    this._send(chunk, encoding, null);
    ret = this._send(crlfBuf, null, callback);
  } else {
    ret = this._send(chunk, encoding, callback);
  }

  return ret;
};


// Stringfy header fields of _headers into _header
OutgoingMessage.prototype._storeHeader = function(statusLine) {
  var state = {
    contentLength: false,
    transferEncoding: false,
  };

  var headerStr = '';

  var keys;
  if (this._headers) {
    keys = Object.keys(this._headers);
    for (var i = 0; i < keys.length; i++) {
      var key = keys[i];
      headerStr += key + ': ' + this._headers[key] + '\r\n';
      this.matchHeader(state, key, this._headers[key]);
    }
  }

  if (!state.contentLength && !state.transferEncoding) {
    // Both Content-Length and Transfer-Encoding are not set by user explicitly
    if (!this._hasBody) {
      // Make sure we don't end the 0\r\n\r\n at the end of the message.
      this.chunkedEncoding = false;
    } else if (!this._removedContentLength &&
               typeof this._contentLength === 'number') {
      headerStr += 'Content-Length: ' + this._contentLength + '\r\n';
    } else if (!this._removedTransferEncoding &&
               this.useChunkedEncodingByDefault) {
      headerStr += 'Transfer-Encoding: chunked\r\n';
      this.chunkedEncoding = true;
    } else {
      // We should only be able to get here if both Content-Length and
      // Transfer-Encoding are removed by the user.
    }
  }

  this._header = statusLine + headerStr;
};


OutgoingMessage.prototype.matchHeader =
function matchHeader(state, field, value) {
  if (field.length < 4 || field.length > 17)
    return;
  field = field.toLowerCase();
  switch (field) {
    case 'transfer-encoding':
      state.transferEncoding = true;
      this._removedTransferEncoding = false;
      if (/(?:^|\W)chunked(?:$|\W)/i.test(value)) {
        this.chunkedEncoding = true;
      }
      break;
    case 'content-length':
      state.contentLength = true;
      this._removedContentLength = false;
      break;
  }
};


OutgoingMessage.prototype.setHeader = function(name, value) {
  if ((typeof name) != 'string') {
    throw new TypeError('Name must be string.');
  }

  if (util.isNullOrUndefined(value)) {
    throw new Error('value required in setHeader(' + name + ', value)');
  }

  if (this._headers === null) {
    this._headers = {};
  }

  this._headers[name.toLowerCase()] = value;
};


OutgoingMessage.prototype.removeHeader = function(name) {
  if (this._headers === null) {
    return;
  }

  delete this._headers[name];

  var key = name.toLowerCase();

  switch (key) {
    case 'transfer-encoding':
      this._removedTransferEncoding = true;
      break;
    case 'content-length':
      this._removedContentLength = true;
      break;
  }
};


OutgoingMessage.prototype.getHeader = function(name) {
  return this._headers[name.toLowerCase()];
};


OutgoingMessage.prototype.getHeaders = function() {
  return this._headers;
};


OutgoingMessage.prototype.setTimeout = function(ms, cb) {
  if (cb)
    this.once('timeout', cb);

  if (!this.socket) {
    this.once('socket', function(socket) {
      socket.setTimeout(ms);
    });
  } else
    this.socket.setTimeout(ms);
};
