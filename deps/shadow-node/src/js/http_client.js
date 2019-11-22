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
var net = require('net');
var stream = require('stream');
var OutgoingMessage = require('http_outgoing').OutgoingMessage;
var common = require('http_common');
var HTTPParser = require('httpparser').HTTPParser;

function ClientRequest(options, cb) {
  OutgoingMessage.call(this);

  this.aborted = undefined;

  // get port, host and method.
  var port = this.port = options.port = options.port || 80;
  var host = this.host = options.host =
    options.hostname ||
    options.host ||
    '127.0.0.1';
  var method = options.method || 'GET';
  var Socket = options.Socket || net.Socket;
  this.path = options.path || '/';

  var methodIsString = (typeof method === 'string');
  if (methodIsString && method) {
    method = this.method = method.toUpperCase();
  } else {
    method = this.method = 'GET';
  }

  if (method === 'GET' ||
      method === 'HEAD' ||
      method === 'DELETE' ||
      method === 'OPTIONS' ||
      method === 'CONNECT') {
    this.useChunkedEncodingByDefault = false;
  } else {
    this.useChunkedEncodingByDefault = true;
  }

  // If `options` contains header information, save it.
  if (options.headers) {
    var keys = Object.keys(options.headers);
    for (var i = 0, l = keys.length; i < l; i++) {
      var key = keys[i];
      this.setHeader(key, options.headers[key]);
    }
  }

  if (host && !this.getHeader('host')) {
    var hostHeader = host;
    if (port && +port !== 80) {
      hostHeader += ':' + port;
    }
    this.setHeader('Host', hostHeader);
  }

  // Register response event handler.
  if (cb) {
    this.once('response', cb);
  }

  // setup connection information.
  this.setupConnection(Socket);
}

util.inherits(ClientRequest, OutgoingMessage);

exports.ClientRequest = ClientRequest;

ClientRequest.prototype.setupConnection = function setupConnection(Socket) {
  // Create socket.
  var socket = new Socket({ port: this.port, host: this.host });

  // connect server.
  socket.connect(this.port, this.host);

  var parser = common.createHTTPParser();
  parser.reinitialize(HTTPParser.RESPONSE);
  socket.parser = parser;
  socket._httpMessage = this;

  parser.socket = socket;
  parser.incoming = null;
  parser._headers = [];
  parser.onIncoming = parserOnIncomingClient;

  this.socket = socket;
  this.connection = socket;
  this.parser = parser;

  socket.on('error', socketOnError);
  socket.on('data', socketOnData);
  socket.on('end', socketOnEnd);
  socket.on('close', socketOnClose);

  // socket emitted when a socket is assigned to req
  process.nextTick(() => {
    this.emit('socket', socket);
  });
};

function cleanUpSocket(socket) {
  var parser = socket.parser;
  var req = socket._httpMessage;

  if (parser) {
    // unref all links to parser, make parser GCed
    parser.finish();
    parser = null;
    socket.parser = null;
    req.parser = null;
  }

  socket.destroy();
}

function emitError(socket, err) {
  var req = socket._httpMessage;

  if (err) {
    var host;
    if (host = req.getHeader('host')) {
      err.message += ': ' + (host ? host : '');
    }
    req.emit('error', err);
  }
}

function socketOnClose() {
  var socket = this;
  var req = socket._httpMessage;

  socket.read();

  req.emit('close');

  if (req.res && req.res.readable) {
    // Socket closed before we emitted 'end' below.
    if (!req.res.complete) req.res.emit('aborted');
    // Socket closed before we emitted 'end'
    var res = req.res;
    res.on('end', function() {
      res.emit('close');
    });
    res.push(null);
  }

  cleanUpSocket(this);
}

function socketOnError(err) {
  cleanUpSocket(this);
  emitError(this, err);
}

function socketOnData(d) {
  var socket = this;
  var req = this._httpMessage;
  var parser = this.parser;

  if (!this._buffer) {
    this._buffer = d;
  } else {
    this._buffer = Buffer.concat([this._buffer, d]);
  }

  if (this._buffer.valid('utf8')) {
    ondata(this._buffer);
    this._buffer = null;
  }

  function ondata(valid) {
    var ret = parser.execute(valid);
    if (ret instanceof Error) {
      cleanUpSocket(socket);
      req.emit('error', ret);
    }
  }
}

function socketOnEnd() {
  cleanUpSocket(this);
}

// This is called by parserOnHeadersComplete after response header is parsed.
// TODO: keepalive support
function parserOnIncomingClient(res, shouldKeepAlive) {
  var socket = this.socket;
  var req = socket._httpMessage;

  if (req.res) {
    // server sent responses twice.
    socket.destroy();
    return false;
  }
  req.res = res;

  res.req = req;

  res.on('end', responseOnEnd);

  req.emit('response', res);

  // response to HEAD req has no body
  var isHeadResponse = (req.method === 'HEAD');
  return isHeadResponse;
}

function responseOnEnd() {
  var res = this;
  var req = res.req;
  var socket = req.socket;

  if (socket._socketState.writable) {
    socket.destroySoon();
  }
}

ClientRequest.prototype._implicitHeader = function _implicitHeader() {
  if (this._sentHeader) {
    throw new Error('Cannot render headers after they are sent');
  }
  this._storeHeader(this.method + ' ' + this.path + ' HTTP/1.1\r\n');
};

ClientRequest.prototype.abort = function() {
  var self = this;
  if (!self.aborted) {
    process.nextTick(function() {
      self.emit('abort');
    });
  }
  // Mark as aborting so we can avoid sending queued request data
  // This is used as a truthy flag elsewhere. The use of Date.now is for
  // debugging purposes only.
  self.aborted = Date.now();

  // If we're aborting, we don't care about any more response data.
  if (self.res) {
    self.res._dump();
  } else {
    self.once('response', function(res) {
      res._dump();
    });
  }

  // the request queue through handling in onSocket.
  if (self.socket) {
    // in-progress
    self.socket._writableState.buffer = [];
    stream.Duplex.prototype.end.call(self.socket, undefined, function() {
      self.socket.destroy();
    });
  }
};

ClientRequest.prototype.setTimeout = function(ms, cb) {
  var self = this;

  if (cb) self.once('timeout', cb);

  function emitTimeout() {
    self.emit('timeout');
  }

  // In IoT.js, socket is already assigned,
  // thus, it is sufficient to trigger timeout on socket 'connect' event.
  this.socket.once('connect', function() {
    self.socket.setTimeout(ms, emitTimeout);
  });

};
