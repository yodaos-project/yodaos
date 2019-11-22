'use strict';

var Pipe = require('pipe_wrap').Pipe;
var EventEmitter = require('events').EventEmitter;
var util = require('util');
var net = require('net');
var constants = require('constants');
var SIGNAL_NO = constants.os.signals;
var INTERNAL_IPC_PAYLOAD_MAX_SIZE = constants.INT32_MAX_VALUE;
var INTERNAL_IPC_HEADER_LENGTH_SIZE = 4;
var INTERNAL_IPC_HEADER_TYPE_SIZE = 1;
var INTERNAL_IPC_HEADER_RESERVED_SIZE = 3;
var INTERNAL_IPC_HEADER_SIZE = INTERNAL_IPC_HEADER_LENGTH_SIZE +
  INTERNAL_IPC_HEADER_TYPE_SIZE +
  INTERNAL_IPC_HEADER_RESERVED_SIZE;

var INTERNAL_IPC_MESSAGE_TYPE = {
  OBJECT: 1,
  STRING: 2
};

function signalName(code) {
  if (!code)
    return null;
  var name;
  var names = Object.keys(SIGNAL_NO);
  for (var i = 0; i < names.length; i++) {
    var curr = names[i];
    if (SIGNAL_NO[curr] === code) {
      name = curr;
      break;
    }
  }
  return name;
}

function ChildProcess() {
  EventEmitter.call(this);
  this._closesNeeded = 1;
  this._closesGot = 0;
  this.connected = false;

  this.signalCode = null;
  this.exitCode = null;
  this.killed = false;
  this.spawnfile = null;

  this._handle = new native.Process();
  this._handle.owner = this;
  this._handle.onexit = function(exitCode, signalCode) {
    if (signalCode) {
      this.signalCode = signalCode;
    } else {
      this.exitCode = exitCode;
    }

    if (this.stdin) {
      this.stdin.destroy();
    }

    this._handle.close();
    this._handle = null;

    if (exitCode < 0) {
      var syscall = this.spawnfile ? 'spawn ' + this.spawnfile : 'spawn';
      var err = new Error(syscall + ' with exit code: ' + exitCode);

      if (this.spawnfile)
        err.path = this.spawnfile;

      err.spawnargs = this.spawnargs.slice(1);
      this.emit('error', err);
    } else {
      this.emit('exit', this.exitCode, signalName(this.signalCode));
    }
    process.nextTick(flushStdio, this);

    // FIXME(Yorkie): use maybeClose later
    // maybeClose(this);
    this.emit('close', this.exitCode, signalName(this.signalCode));
  }.bind(this);
}
util.inherits(ChildProcess, EventEmitter);

function flushStdio(subprocess) {
  var stdio = subprocess.stdio;
  if (stdio == null) return;

  for (var i = 0; i < stdio.length; i++) {
    var stream = stdio[i];
    if (!stream || !stream.readable || stream._readableState.readableListening)
      continue;
    stream.resume();
  }
}

function createSocket(pipe, readable) {
  var s = new net.Socket({ handle: pipe });
  if (readable) {
    s.writable = false;
    s.readable = true;
  } else {
    s.writable = true;
    s.readable = false;
  }
  return s;
}

function onErrorNT(self, err) {
  self._handle.onexit(err);
}

ChildProcess.prototype.kill = function(sig) {
  var signal;
  if (typeof sig === 'number') {
    signal = sig;
  } else if (SIGNAL_NO[sig]) {
    signal = SIGNAL_NO[sig];
  } else {
    signal = SIGNAL_NO.SIGTERM;
  }
  if (this._handle) {
    var err = this._handle.kill(signal);
    if (err === 0) {
      /* Success. */
      this.killed = true;
      return true;
    }
    this.emit('error', new Error('kill process failed'));
  }
  /* Kill didn't succeed. */
  return false;
};

ChildProcess.prototype.ref = function() {
  if (this._handle) this._handle.ref();
};

ChildProcess.prototype.unref = function() {
  if (this._handle) this._handle.unref();
};

ChildProcess.prototype.spawn = function(options) {
  var ipc;
  var ipcFd;
  var i;

  if (options === null || typeof options !== 'object') {
    throw new TypeError('ERR_INVALID_ARG_TYPE');
  }

  // If no `stdio` option was given - use default
  var stdio = options.stdio || 'pipe';
  stdio = _validateStdio(stdio, false);

  ipc = stdio.ipc;
  ipcFd = stdio.ipcFd;
  stdio = options.stdio = stdio.stdio;

  if (ipc !== undefined) {
    // Let child process know about opened IPC channel
    if (options.envPairs === undefined)
      options.envPairs = [];
    else if (!Array.isArray(options.envPairs)) {
      throw new TypeError('ERR_INVALID_ARG_TYPE', 'options.envPairs');
    }

    options.envPairs.push('NODE_CHANNEL_FD=' + ipcFd);
  }

  if (typeof options.file !== 'string') {
    throw new TypeError('ERR_INVALID_ARG_TYPE');
  }
  this.spawnfile = options.file;

  if (Array.isArray(options.args))
    this.spawnargs = options.args;
  else if (options.args === undefined)
    this.spawnargs = [];
  else
    throw new TypeError('ERR_INVALID_ARG_TYPE');

  var err = this._handle.spawn(options);
  if (err) {
    process.nextTick(onErrorNT, this, err);
    // return err;
  }

  this.pid = this._handle.pid;
  for (i = 0; i < stdio.length; i++) {
    var stream = stdio[i];
    if (stream.type === 'ignore')
      continue;

    if (stream.ipc) {
      this._closesNeeded++;
      continue;
    }

    if (stream.handle) {
      // when i === 0 - we're dealing with stdin
      // (which is the only one writable pipe)
      stream.socket = createSocket(this.pid !== 0 ?
        stream.handle : null, i > 0);

      if (i > 0 && this.pid !== 0) {
        this._closesNeeded++;
        stream.socket.on('close', function() {
          maybeClose(this);
        }.bind(this));
      }
    }
  }

  this.stdin = stdio.length >= 1 && stdio[0].socket !== undefined ?
    stdio[0].socket : null;
  this.stdout = stdio.length >= 2 && stdio[1].socket !== undefined ?
    stdio[1].socket : null;
  this.stderr = stdio.length >= 3 && stdio[2].socket !== undefined ?
    stdio[2].socket : null;

  this.stdio = [];

  for (i = 0; i < stdio.length; i++)
    this.stdio.push(stdio[i].socket === undefined ? null : stdio[i].socket);

  // Add .send() method and start listening for IPC data
  if (ipc !== undefined) setupChannel(this, ipc);

  return err;
};

function getHandleWrapType(stream) {
  if (stream instanceof Pipe) return 'pipe';
  return false;
}

function _validateStdio(stdio, sync) {
  var ipc;
  var ipcFd;

  // Replace shortcut with an array
  if (typeof stdio === 'string') {
    switch (stdio) {
      case 'ignore': stdio = ['ignore', 'ignore', 'ignore']; break;
      case 'pipe': stdio = ['pipe', 'pipe', 'pipe']; break;
      case 'inherit': stdio = [0, 1, 2]; break;
      default:
        throw new TypeError('ERR_INVALID_OPT_VALUE with stdio=' + stdio);
    }
  } else if (!Array.isArray(stdio)) {
    throw new TypeError('ERR_INVALID_OPT_VALUE');
  }

  // At least 3 stdio will be created
  // Don't concat() a new Array() because it would be sparse, and
  // stdio.reduce() would skip the sparse elements of stdio.
  // See http://stackoverflow.com/a/5501711/3561
  while (stdio.length < 3) stdio.push(undefined);

  // Translate stdio into C++-readable form
  // (i.e. PipeWraps or fds)
  stdio = stdio.reduce(function(acc, stdio, i) {
    function cleanup() {
      for (var i = 0; i < acc.length; i++) {
        if ((acc[i].type === 'pipe' || acc[i].type === 'ipc') && acc[i].handle)
          acc[i].handle.close();
      }
    }

    // Defaults
    if (stdio == null) {
      stdio = i < 3 ? 'pipe' : 'ignore';
    }

    if (stdio === 'ignore') {
      acc.push({ type: 'ignore' });
    } else if (stdio === 'pipe' || typeof stdio === 'number' && stdio < 0) {
      var a = {
        type: 'pipe',
        readable: i === 0,
        writable: i !== 0
      };

      if (!sync) {
        a.handle = new Pipe();
      }

      acc.push(a);
    } else if (stdio === 'ipc') {
      if (sync || ipc !== undefined) {
        // Cleanup previously created pipes
        cleanup();
        if (!sync)
          throw new Error('ERR_IPC_ONE_PIPE');
        else
          throw new Error('ERR_IPC_SYNC_FORK');
      }

      ipc = new Pipe(true);
      ipcFd = i;

      acc.push({
        type: 'pipe',
        handle: ipc,
        ipc: true
      });
    } else if (stdio === 'inherit') {
      acc.push({
        type: 'inherit',
        fd: i
      });
    } else if (typeof stdio === 'number' || typeof stdio.fd === 'number') {
      acc.push({
        type: 'fd',
        fd: typeof stdio === 'number' ? stdio : stdio.fd
      });
    } else if (getHandleWrapType(stdio) || getHandleWrapType(stdio.handle) ||
               getHandleWrapType(stdio._handle)) {
      var handle = getHandleWrapType(stdio) ?
        stdio :
        getHandleWrapType(stdio.handle) ? stdio.handle : stdio._handle;

      acc.push({
        type: 'wrap',
        wrapType: getHandleWrapType(handle),
        handle: handle
      });
    } else if (typeof stdio === 'string') {
      if (!sync) {
        cleanup();
        throw new TypeError('ERR_INVALID_SYNC_FORK_INPUT');
      }
    } else {
      // Cleanup
      cleanup();
      throw new TypeError('ERR_INVALID_OPT_VALUE');
    }
    return acc;
  }, []);

  return {
    stdio: stdio,
    ipc: ipc,
    ipcFd: ipcFd,
  };
}

function normalizeSpawnArguments(file, args, options) {
  if (typeof file !== 'string' || file.length === 0)
    throw new TypeError('"file" argument must be a non-empty string');

  if (Array.isArray(args)) {
    args = args.slice(0);
  } else if (args !== undefined &&
             (args === null || typeof args !== 'object')) {
    throw new TypeError('Incorrect value of args option');
  } else {
    options = args;
    args = [];
  }

  if (options === undefined)
    options = {};
  else if (options === null || typeof options !== 'object')
    throw new TypeError('"options" argument must be an object');

  // Validate the cwd, if present.
  if (options.cwd != null &&
      typeof options.cwd !== 'string') {
    throw new TypeError('"cwd" must be a string');
  }

  // Validate detached, if present.
  if (options.detached != null &&
      typeof options.detached !== 'boolean') {
    throw new TypeError('"detached" must be a boolean');
  }

  // Validate the uid, if present.
  if (options.uid != null && !Number.isInteger(options.uid)) {
    throw new TypeError('"uid" must be an integer');
  }

  // Validate the gid, if present.
  if (options.gid != null && !Number.isInteger(options.gid)) {
    throw new TypeError('"gid" must be an integer');
  }

  // Validate the shell, if present.
  if (options.shell != null &&
      typeof options.shell !== 'boolean' &&
      typeof options.shell !== 'string') {
    throw new TypeError('"shell" must be a boolean or string');
  }

  // Validate argv0, if present.
  if (options.argv0 != null &&
      typeof options.argv0 !== 'string') {
    throw new TypeError('"argv0" must be a string');
  }

  // Make a shallow copy so we don't clobber the user's options object.
  options = Object.assign({}, options);

  if (options.shell) {
    var command = [file].concat(args).join(' ');

    if (process.platform === 'win32') {
      throw new Error('not supported for windows');
    } else {
      if (typeof options.shell === 'string')
        file = options.shell;
      else if (process.platform === 'android')
        file = '/system/bin/sh';
      else
        file = '/bin/sh';
      args = ['-c', command];
    }
  }

  if (typeof options.argv0 === 'string') {
    args.unshift(options.argv0);
  } else {
    args.unshift(file);
  }

  var env = options.env || process.env;
  var envPairs = [];

  for (var key in env) {
    envPairs.push(key + '=' + env[key]);
  }

  return {
    file: file,
    args: args,
    options: options,
    envPairs: envPairs
  };
}

function maybeClose(subprocess) {
  subprocess._closesGot++;

  if (subprocess._closesGot === subprocess._closesNeeded) {
    subprocess.emit('close', subprocess.exitCode, subprocess.signalCode);
  }
}

function Control(channel) {
  EventEmitter.call(this);
  this.channel = channel;
  this.refs = 0;
}
util.inherits(Control, EventEmitter);

Control.prototype.ref = function() {
  if (++this.refs === 1) {
    this.channel.ref();
  }
};

Control.prototype.unref = function() {
  if (--this.refs === 0) {
    this.channel.unref();
    this.emit('unref');
  }
};

function setupChannel(target, channel) {
  target.channel = channel;

  // _channel can be deprecated in version 8
  Object.defineProperty(target, '_channel', {
    get: function() {
      return target.channel;
    },
    set: function(val) {
      target.channel = val;
    },
    enumerable: true
  });

  var control = new Control(channel);
  channel.pendingBuffer = null;
  channel.buffering = false;
  channel.onread = function(socket, nread, isEOF, buffer) {
    if (nread > 0) {
      if (channel.pendingBuffer) {
        channel.pendingBuffer = Buffer.concat([channel.pendingBuffer, buffer]);
      } else {
        channel.pendingBuffer = buffer;
      }
      var bufLength = channel.pendingBuffer.byteLength;
      var headerOffset = 0;
      while (bufLength - headerOffset >= INTERNAL_IPC_HEADER_SIZE) {
        var dataSize = channel.pendingBuffer.readInt32BE(headerOffset);
        var typeOffset = headerOffset + INTERNAL_IPC_HEADER_LENGTH_SIZE;
        var dataType = channel.pendingBuffer.readInt32BE(typeOffset);
        var dataBegin = headerOffset + INTERNAL_IPC_HEADER_SIZE;
        var dataEnd = dataBegin + dataSize;
        if (bufLength < dataEnd) {
          // package is a incomplete message
          break;
        }
        headerOffset = dataEnd;
        var message = channel.pendingBuffer.toString(dataBegin, dataEnd);
        if (dataType === INTERNAL_IPC_MESSAGE_TYPE.OBJECT) {
          message = JSON.parse(message);
        }
        handleMessage(message, undefined);
      }
      if (headerOffset === bufLength) {
        // a packet is a complete message
        channel.pendingBuffer = null;
        channel.buffering = false;
      } else {
        // sticky packet, pending left buffer
        channel.pendingBuffer =
          channel.pendingBuffer.slice(headerOffset, bufLength);
        channel.buffering = true;
      }
    } else {
      this.buffering = false;
      target.disconnect();
      channel.onread = function() {};
      channel.close();
      target.channel = null;
      maybeClose(target);
    }

  };

  // object where socket lists will live
  channel.sockets = { got: {}, send: {} };

  target.send = function(message, handle, options, callback) {
    if (typeof handle === 'function') {
      callback = handle;
      handle = undefined;
      options = undefined;
    } else if (typeof options === 'function') {
      callback = options;
      options = undefined;
    } else if (options !== undefined &&
               (options === null || typeof options !== 'object')) {
      throw new TypeError('ERR_INVALID_ARG_TYPE');
    }

    options = Object.assign({ swallowErrors: false }, options);

    if (!this.connected) {
      var ex = new Error('ERR_IPC_CHANNEL_CLOSED');
      if (typeof callback === 'function') {
        process.nextTick(callback, ex);
      } else {
        process.nextTick(function() {
          this.emit('error', ex);
        }.bind(this));
      }
      return false;
    }
    if (message === undefined)
      throw new TypeError('ERR_MISSING_ARGS');

    // Support legacy function signature
    if (typeof options === 'boolean') {
      options = { swallowErrors: options };
    }
    var messageType;
    if (typeof message === 'object') {
      messageType = INTERNAL_IPC_MESSAGE_TYPE.OBJECT;
      try {
        message = JSON.stringify(message);
      } catch (err) {
        throw new Error('ERR_IPC_MESSAGE_SERIALIZE_ERROR');
      }
    } else {
      messageType = INTERNAL_IPC_MESSAGE_TYPE.STRING;
      if (typeof message !== 'string') {
        if (message.toString) {
          message = message.toString();
        } else {
          throw new Error('ERR_IPC_MESSAGE_TYPE_INVALID');
        }
      }
    }
    var messageByteLength = Buffer.byteLength(message);
    if (messageByteLength > INTERNAL_IPC_PAYLOAD_MAX_SIZE) {
      callback(new Error('ERR_MESSAGE_TOO_LARGE'));
      return false;
    }
    var dataByteLength = INTERNAL_IPC_HEADER_SIZE + messageByteLength;
    var buffer = Buffer.allocUnsafe(dataByteLength);
    buffer.writeInt32BE(messageByteLength, 0);
    buffer.writeInt32BE(messageType, INTERNAL_IPC_HEADER_LENGTH_SIZE);
    buffer.write(message, INTERNAL_IPC_HEADER_SIZE);
    channel.write(buffer, callback);
    return true;
  };

  // connected will be set to false immediately when a disconnect() is
  // requested, even though the channel might still be alive internally to
  // process queued messages. The three states are distinguished as follows:
  // - disconnect() never requested: channel is not null and connected
  //   is true
  // - disconnect() requested, messages in the queue: channel is not null
  //   and connected is false
  // - disconnect() requested, channel actually disconnected: channel is
  //   null and connected is false
  target.connected = true;

  target.disconnect = function() {
    if (!this.connected) {
      this.emit('error', new Error('ERR_IPC_DISCONNECTED'));
      return;
    }

    // Do not allow any new messages to be written.
    this.connected = false;

    this._disconnect();
  };

  target._disconnect = function() {
    // This marks the fact that the channel is actually disconnected.
    this.channel = null;

    var fired = false;
    function finish() {
      if (fired) return;
      fired = true;

      channel.close();
      target.emit('disconnect');
    }

    // If a message is being read, then wait for it to complete.
    if (channel.buffering) {
      this.once('message', finish);

      return;
    }

    process.nextTick(finish);
  };

  function emit(event, message, handle) {
    target.emit(event, message, handle);
  }

  function handleMessage(message, handle) {
    if (!target.channel)
      return;
    process.nextTick(emit, 'message', message, handle);
  }

  channel.readStart();
  return control;
}

var spawn = exports.spawn = function(/* file, args, options */) {
  var opts = normalizeSpawnArguments.apply(null, arguments);
  var options = opts.options;
  var child = new ChildProcess();
  child.spawn({
    file: opts.file,
    args: opts.args,
    cwd: options.cwd,
    // TODO(Yorkie): not supported for windows option
    // windowsVerbatimArguments: !!options.windowsVerbatimArguments,
    detached: !!options.detached,
    envPairs: opts.envPairs,
    stdio: options.stdio,
    uid: options.uid,
    gid: options.gid
  });
  return child;
};

function stdioStringToArray(option) {
  switch (option) {
    case 'ignore':
    case 'pipe':
    case 'inherit':
      return [option, option, option, 'ipc'];
    default:
      throw new TypeError('Incorrect value of stdio option: ' + option);
  }
}

exports.fork = function(modulePath /* , args, options */) {
  // Get options and args arguments.
  var execArgv;
  var options = {};
  var args = [];
  var pos = 1;
  if (pos < arguments.length && Array.isArray(arguments[pos])) {
    args = arguments[pos++];
  }

  if (pos < arguments.length && arguments[pos] != null) {
    if (typeof arguments[pos] !== 'object') {
      throw new TypeError('Incorrect value of args option');
    }

    options = Object.assign({}, arguments[pos++]);
  }

  // Prepare arguments for fork:
  execArgv = options.execArgv || process.execArgv;

  if (execArgv === process.execArgv && process._eval != null) {
    var index = execArgv.lastIndexOf(process._eval);
    if (index > 0) {
      // Remove the -e switch to avoid fork bombing ourselves.
      execArgv = execArgv.slice();
      execArgv.splice(index - 1, 2);
    }
  }

  args = execArgv.concat([modulePath], args);

  if (typeof options.stdio === 'string') {
    options.stdio = stdioStringToArray(options.stdio);
  } else if (!Array.isArray(options.stdio)) {
    // Use a separate fd=3 for the IPC channel. Inherit stdin, stdout,
    // and stderr from the parent if silent isn't set.
    options.stdio = options.silent ? stdioStringToArray('pipe') :
      stdioStringToArray('inherit');
  } else if (options.stdio.indexOf('ipc') === -1) {
    throw new TypeError('Forked processes must have an IPC channel');
  }

  options.execPath = options.execPath || process.execPath;

  return spawn(options.execPath, args, options);
};

function normalizeExecArgs(command, options, callback) {
  if (typeof options === 'function') {
    callback = options;
    options = undefined;
  }

  // Make a shallow copy so we don't clobber the user's options object.
  options = Object.assign({}, options);
  options.shell = typeof options.shell === 'string' ? options.shell : true;

  return {
    file: command,
    options: options,
    callback: callback
  };
}

exports.exec = function(command /* , options, callback */) {
  var opts = normalizeExecArgs.apply(null, arguments);
  return exports.execFile(opts.file,
                          opts.options,
                          opts.callback);
};

exports.execFile = function(file /* , args, options, callback */) {
  var args = [];
  var callback;
  var options = {
    encoding: 'utf8',
    timeout: 0,
    maxBuffer: 200 * 1024,
    killSignal: 'SIGTERM',
    cwd: null,
    env: null,
    shell: false
  };

  // Parse the optional positional parameters.
  var pos = 1;
  if (pos < arguments.length && Array.isArray(arguments[pos])) {
    args = arguments[pos++];
  } else if (pos < arguments.length && arguments[pos] == null) {
    pos++;
  }

  if (pos < arguments.length && typeof arguments[pos] === 'object') {
    options = Object.assign(options, arguments[pos++]);
  } else if (pos < arguments.length && arguments[pos] == null) {
    pos++;
  }

  if (pos < arguments.length && typeof arguments[pos] === 'function') {
    callback = arguments[pos++];
  }

  if (!callback && pos < arguments.length && arguments[pos] != null) {
    throw new TypeError('Incorrect value of args option');
  }

  // Validate the timeout, if present.
  validateTimeout(options.timeout);

  // Validate maxBuffer, if present.
  validateMaxBuffer(options.maxBuffer);

  options.killSignal = options.killSignal;

  var child = spawn(file, args, {
    cwd: options.cwd,
    env: options.env,
    gid: options.gid,
    uid: options.uid,
    shell: options.shell,
  });

  var encoding;
  var _stdout;
  var _stderr;
  if (options.encoding !== 'buffer') {
    encoding = options.encoding;
    _stdout = '';
    _stderr = '';
  } else {
    _stdout = [];
    _stderr = [];
    encoding = null;
  }
  var stdoutLen = 0;
  var stderrLen = 0;
  var killed = false;
  var exited = false;
  var timeoutId;

  var ex = null;
  var cmd = file;

  function exithandler(code, signal) {
    if (exited) return;
    exited = true;

    if (timeoutId) {
      clearTimeout(timeoutId);
      timeoutId = null;
    }

    if (!callback) return;

    // merge chunks
    var stdout;
    var stderr;
    if (encoding) {
      stdout = _stdout;
      stderr = _stderr;
    } else {
      stdout = Buffer.concat(_stdout);
      stderr = Buffer.concat(_stderr);
    }

    if (!ex && code === 0 && signal === null) {
      callback(null, stdout, stderr);
      return;
    }

    if (args.length !== 0)
      cmd += ' ' + args.join(' ');

    if (!ex) {
      ex = new Error('Command failed: ' + cmd + '\n' + stderr);
      ex.killed = child.killed || killed;
      // TODO(Yorkie): implement errname()
      // ex.code = code < 0 ? errname(code) : code;
      ex.code = code;
      ex.signal = signal;
    }

    ex.cmd = cmd;
    callback(ex, stdout, stderr);
  }

  function errorhandler(e) {
    ex = e;

    if (child.stdout)
      child.stdout.destroy();

    if (child.stderr)
      child.stderr.destroy();

    exithandler();
  }

  function kill() {
    if (child.stdout)
      child.stdout.destroy();

    if (child.stderr)
      child.stderr.destroy();

    killed = true;
    try {
      child.kill(options.killSignal);
    } catch (e) {
      ex = e;
      exithandler();
    }
  }

  if (options.timeout > 0) {
    timeoutId = setTimeout(function delayedKill() {
      kill();
      timeoutId = null;
    }, options.timeout);
  }

  if (child.stdout) {
    if (encoding)
      child.stdout.setEncoding(encoding);

    child.stdout.on('data', function onChildStdout(chunk) {
      stdoutLen += chunk.length;

      if (stdoutLen > options.maxBuffer) {
        ex = new Error('stdout maxBuffer exceeded');
        kill();
      } else if (encoding)
        _stdout += chunk;
      else
        _stdout.push(chunk);
    });
  }

  if (child.stderr) {
    if (encoding)
      child.stderr.setEncoding(encoding);

    child.stderr.on('data', function onChildStderr(chunk) {
      stderrLen += chunk.length;

      if (stderrLen > options.maxBuffer) {
        ex = new Error('stderr maxBuffer exceeded');
        kill();
      } else if (encoding)
        _stderr += chunk;
      else
        _stderr.push(chunk);
    });
  }

  child.addListener('close', exithandler);
  child.addListener('error', errorhandler);
  return child;
};

exports._forkChild = function(fd) {
  var p = new Pipe(true);
  p.open(fd);
  p.unref();
  setupChannel(process, p);
};

exports.ChildProcess = ChildProcess;

function validateTimeout(timeout) {
  if (timeout != null && timeout < 0) {
    throw new TypeError('"timeout" must be an unsigned integer');
  }
}

function validateMaxBuffer(maxBuffer) {
  if (maxBuffer != null && !(typeof maxBuffer === 'number' && maxBuffer >= 0)) {
    throw new TypeError('"maxBuffer" must be a positive number');
  }
}
