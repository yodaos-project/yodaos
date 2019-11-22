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

function IncomingMessage(socket) {
  stream.Readable.call(this);

  this.socket = socket;
  this.connection = socket;

  this.readable = true;

  this.headers = {};

  this.complete = false;

  this._dumped = false;

  // for request (server)
  this.url = '';
  this.method = null;

  // for response (client)
  this.statusCode = null;
  this.statusMessage = null;

}

util.inherits(IncomingMessage, stream.Readable);

exports.IncomingMessage = IncomingMessage;

// 'array' header list is taken from:
// https://mxr.mozilla.org/mozilla/source/netwerk/protocol/http/src/nsHttpHeaderArray.cpp
function matchKnownFields(field) {
  while (true) {
    switch (field) {
      case 'Content-Type':
      case 'content-type':
        return 'content-type';
      case 'Content-Length':
      case 'content-length':
        return 'content-length';
      case 'User-Agent':
      case 'user-agent':
        return 'user-agent';
      case 'Referer':
      case 'referer':
        return 'referer';
      case 'Host':
      case 'host':
        return 'host';
      case 'Authorization':
      case 'authorization':
        return 'authorization';
      case 'Proxy-Authorization':
      case 'proxy-authorization':
        return 'proxy-authorization';
      case 'If-Modified-Since':
      case 'if-modified-since':
        return 'if-modified-since';
      case 'If-Unmodified-Since':
      case 'if-unmodified-since':
        return 'if-unmodified-since';
      case 'From':
      case 'from':
        return 'from';
      case 'Location':
      case 'location':
        return 'location';
      case 'Max-Forwards':
      case 'max-forwards':
        return 'max-forwards';
      case 'Retry-After':
      case 'retry-after':
        return 'retry-after';
      case 'ETag':
      case 'etag':
        return 'etag';
      case 'Last-Modified':
      case 'last-modified':
        return 'last-modified';
      case 'Server':
      case 'server':
        return 'server';
      case 'Age':
      case 'age':
        return 'age';
      case 'Expires':
      case 'expires':
        return 'expires';
      case 'Set-Cookie':
      case 'set-cookie':
        return 'set-cookie';
      case 'Cookie':
      case 'cookie':
        return 'cookie';
      case 'Transfer-Encoding':
      case 'transfer-encoding':
        return 'transfer-encoding';
      case 'Date':
      case 'date':
        return 'date';
      case 'Connection':
      case 'connection':
        return 'connection';
      case 'Cache-Control':
      case 'cache-control':
        return 'cache-control';
      case 'Vary':
      case 'vary':
        return 'vary';
      case 'Content-Encoding':
      case 'content-encoding':
        return 'content-encoding';
      case 'Origin':
      case 'origin':
        return 'origin';
      case 'Upgrade':
      case 'upgrade':
        return 'upgrade';
      case 'Expect':
      case 'expect':
        return 'expect';
      case 'If-Match':
      case 'if-match':
        return 'if-match';
      case 'If-None-Match':
      case 'if-none-match':
        return 'if-none-match';
      case 'Accept':
      case 'accept':
        return 'accept';
      case 'Accept-Encoding':
      case 'accept-encoding':
        return 'accept-encoding';
      case 'Accept-Language':
      case 'accept-language':
        return 'accept-language';
      case 'X-Forwarded-For':
      case 'x-forwarded-for':
        return 'x-forwarded-for';
      case 'X-Forwarded-Host':
      case 'x-forwarded-host':
        return 'x-forwarded-host';
      case 'X-Forwarded-Proto':
      case 'x-forwarded-proto':
        return 'x-forwarded-proto';
      default:
        return field.toLowerCase();
    }
  }
}

IncomingMessage.prototype.read = function(n) {
  this.read = stream.Readable.prototype.read;
  return this.read(n);
};


IncomingMessage.prototype.addHeaders = function(headers) {
  if (!this.headers) {
    this.headers = {};
  }

  var key;
  // FIXME: handle headers as array if array C API is done.
  for (var i = 0; i < headers.length; i = i + 2) {
    key = matchKnownFields(headers[i]);
    this.headers[key] = headers[i + 1];
  }
};


IncomingMessage.prototype.setTimeout = function(ms, cb) {
  if (cb)
    this.once('timeout', cb);
  this.socket.setTimeout(ms, cb);
};


IncomingMessage.prototype._dump = function() {
  if (!this._dumped) {
    this._dumped = true;
    // If there is buffered data, it may trigger 'data' events.
    // Remove 'data' event listeners explicitly.
    this.removeAllListeners('data');
    this.resume();
  }
};
