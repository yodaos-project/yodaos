'use strict';

require('crypto');

var net = require('net');
var tcp = require('tcp');
var util = require('util');
var TlsWrap = native.TlsWrap;
var TLS_CHUNK_MAX_SIZE = require('constants').TLS_CHUNK_MAX_SIZE;

function Delegator(proto, target) {
  this.proto = proto;
  this.target = target;
}

Delegator.prototype.method = function method(name) {
  var proto = this.proto;
  var target = this.target;

  proto[name] = function delegation() {
    return this[target][name].apply(this[target], arguments);
  };

  return this;
};

function TLSHandle(options) {
  var self = this;
  this.tcpHandle = new tcp();
  this.tlsWrap = new TlsWrap(options);
  this.tlsWrap.onwrite = function onwrite(chunk) {
    self.tcpHandle.write(chunk, function afterWrite(status) {
      /**
       * Empty stub callback for native calls,
       * for tlsWrap's actual ignoring of when the chunk that is not written by
       * user explicitly is written
       */
    });
  };
  this.tlsWrap.onread = function onread(nread, buffer) {
    if (self.onread) {
      return self.onread(self.tlsWrap.jsref, nread, self._EOF === true, buffer);
    }
  };
  this.tcpHandle.onclose = this.tlsWrap.onclose = function onclose() {
    if (self.onclose) {
      self.onclose();
    }
  };

  Object.defineProperty(this, 'owner', {
    get: function get() {
      return self.tcpHandle.owner;
    },
    set: function set(val) {
      self.tlsWrap.jsref = val;
      self.tcpHandle.owner = val;
    }
  });

  this.tcpHandle.onread = function onread(socket, nread, isEOF, buffer) {
    if (nread < 0) {
      if (isEOF) {
        self._EOF = true;
      }
      if (self.onread) {
        self.onread(socket, nread, isEOF, buffer);
      }
      return;
    }
    self.tlsWrap.read(buffer);
  };
}

new Delegator(TLSHandle.prototype, 'tcpHandle')
  .method('shutdown')
  .method('setKeepAlive')
  .method('getpeername')
  .method('getsockname');

TLSHandle.prototype.connect = function connect(ip, port, afterConnect) {
  var self = this;
  this.tcpHandle.connect(ip, port, function tlsAfterConnect(status) {
    if (status !== 0) {
      return afterConnect(status);
    }
    self.tlsWrap.onhandshakedone = function onhandshakedone() {
      self.authorized = true;
      self.encrypted = true;

      afterConnect(0);
    };

    self.tcpHandle.readStart();
    var err = self.tlsWrap.handshake();
    if (err < 0) {
      throw new Error(TlsWrap.errname(err));
    }
  });
};

TLSHandle.prototype.close = function close() {
  this.tcpHandle.close();
  this.tlsWrap.end();
};

TLSHandle.prototype.readStart = function readStart() {
  /**
   * Underlying TcpHandle#readStart is called in TLSHandle#connect
   * since the handshake has been automatically initiated by TLSHandle
   * TLSHandle#readStart has nothing to do than a dummy function
   */
};

TLSHandle.prototype.write = function write(data, callback) {
  if (!Buffer.isBuffer(data))
    data = new Buffer(data);

  if (!this.encrypted) {
    this._writev.push([data, callback]);
    return;
  }

  var chunks = [];
  var chunkByteLength = data.byteLength;
  var sourceStart = 0;
  var sourceLength;
  while (sourceStart < chunkByteLength) {
    var chunkLeft = chunkByteLength - sourceStart;
    if (chunkLeft > TLS_CHUNK_MAX_SIZE) {
      sourceLength = TLS_CHUNK_MAX_SIZE;
    } else {
      sourceLength = chunkLeft;
    }
    var chunk = data.slice(sourceStart, sourceStart + sourceLength);
    sourceStart += sourceLength;
    try {
      /* XXX: tls.write may throw error if iotjs_tlswrap_encode_data failure */
      var encodedChunk = this.tlsWrap.write(chunk);
      chunks.push(encodedChunk);
    } catch (err) {
      callback(err);
      return false;
    }
  }
  var encodedData = Buffer.concat(chunks);
  this.tcpHandle.write(encodedData, callback);
};

function TLSSocket(socket, opts) {
  if (!(this instanceof TLSSocket))
    return new TLSSocket(socket, opts);

  // init the handle
  var tlsOptions = Object.assign({
    servername: socket.host || socket.hostname,
    // rejectUnauthorized: false,
  }, opts);

  // TODO: currently certification is disabled
  tlsOptions.rejectUnauthorized = false;

  // handle the [ca1,ca2,...]
  if (Array.isArray(tlsOptions.ca)) {
    tlsOptions.ca = tlsOptions.ca.join('\n');
  }

  var handle = new TLSHandle(tlsOptions);

  net.Socket.call(this, Object.assign(socket, { handle: handle }));

  this.servername = tlsOptions.servername;
  this.authorized = false;
  this.authorizationError = null;
  this._writev = [];

  // Just a documented property to make secure sockets
  // distinguishable from regular ones.
  this.encrypted = false;
}

util.inherits(TLSSocket, net.Socket);

function connect(options, callback) {
  return TLSSocket({
    port: options.port,
    host: options.host,
  }, options).connect(options, callback);
}

exports.TLSSocket = TLSSocket;
exports.connect = connect;
