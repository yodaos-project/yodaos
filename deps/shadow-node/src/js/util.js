'use strict';

function isNull(arg) {
  return arg === null;
}


function isDate(arg) {
  return arg instanceof Date;
}


function isError(arg) {
  return arg instanceof Error;
}


function isUndefined(arg) {
  return arg === undefined;
}


function isNullOrUndefined(arg) {
  return arg === null || arg === undefined;
}


function isNumber(arg) {
  return typeof arg === 'number';
}

function isFinite(arg) {
  return (arg === 0) || (arg !== arg / 2);
}

function isBoolean(arg) {
  return typeof arg === 'boolean';
}


function isString(arg) {
  return typeof arg === 'string';
}


function isObject(arg) {
  return typeof arg === 'object' && arg != null;
}


function isFunction(arg) {
  return typeof arg === 'function';
}


function isBuffer(arg) {
  return arg instanceof Buffer;
}

function isRegExp(arg) {
  return arg instanceof RegExp;
}


function inherits(ctor, superCtor) {
  ctor.prototype = Object.create(superCtor.prototype, {
    constructor: {
      value: ctor,
      enumerable: false,
      writable: true,
      configurable: true,
    },
  });
}


function format(s) {
  if (!isString(s)) {
    var arrs = [];
    for (var i0 = 0; i0 < arguments.length; ++i0) {
      arrs.push(formatValue(arguments[i0]));
    }
    return arrs.join(' ');
  }

  var i = 1;
  var args = arguments;
  var arg_string;
  var str = '';
  var start = 0;
  var end = 0;

  while (end < s.length) {
    if (s.charAt(end) !== '%') {
      end++;
      continue;
    }

    str += s.slice(start, end);

    switch (s.charAt(end + 1)) {
      case 's':
        arg_string = String(args[i]);
        break;
      case 'd':
        arg_string = Number(args[i]);
        break;
      case 'j':
        try {
          arg_string = JSON.stringify(args[i]);
        } catch (_) {
          arg_string = '[Circular]';
        }
        break;
      case '%':
        str += '%';
        start = end = end + 2;
        continue;
      default:
        str = str + '%' + s.charAt(end + 1);
        start = end = end + 2;
        continue;
    }

    if (i >= args.length) {
      str = str + '%' + s.charAt(end + 1);
    } else {
      i++;
      str += arg_string;
    }

    start = end = end + 2;
  }

  str += s.slice(start, end);

  while (i < args.length) {
    str += ' ' + formatValue(args[i++]);
  }

  return str;
}

function formatValue(v) {
  if (v === undefined)
    return 'undefined';
  if (v === null)
    return 'null';

  if (v instanceof Error) {
    return v.name + ': ' + v.message + '\n' + v.stack;
  }

  if (v instanceof Date) {
    return v.toString();
  }

  if (Buffer.isBuffer(v)) {
    return v.inspect();
  }
  if (Array.isArray(v)) {
    return '[ ' + v.map(formatValue).join(', ') + ' ]';
  }
  if (typeof v === 'object') {
    return JSON.stringify(v, jsonReplacer, 2);
  }
  return v.toString();
}


function jsonReplacer(key, value) {
  if (Buffer.isBuffer(value))
    return value.toString('utf8');
  else
    return value;
}


function stringToNumber(value, default_value) {
  var num = Number(value);
  return isNaN(num) ? default_value : num;
}


function errnoException(err, syscall, original) {
  var errname = 'error'; // uv.errname(err);
  var message = syscall + ' ' + errname;

  if (original)
    message += ' ' + original;

  var e = new Error(message);
  e.code = errname;
  e.errno = errname;
  e.syscall = syscall;

  return e;
}


function exceptionWithHostPort(err, syscall, address, port, additional) {
  var details;
  if (port && port > 0) {
    details = address + ':' + port;
  } else {
    details = address;
  }

  if (additional) {
    details += ' - Local (' + additional + ')';
  }

  var ex = exports.errnoException(err, syscall, details);
  ex.address = address;
  if (port) {
    ex.port = port;
  }

  return ex;
}

var codesWarned = {};

// Mark that a method should not be used.
// Returns a modified function which warns once by default.
// If --no-deprecation is set, then it is a no-op.
function deprecate(fn, msg, code) {
  if (process.noDeprecation === true) {
    return fn;
  }

  if (code !== undefined && typeof code !== 'string')
    throw new TypeError('ERR_INVALID_ARG_TYPE');

  var warned = false;
  function deprecated() {
    if (!warned) {
      warned = true;
      if (code !== undefined) {
        if (!codesWarned[code]) {
          process.emitWarning(msg, 'DeprecationWarning', code, deprecated);
          codesWarned[code] = true;
        }
      } else {
        process.emitWarning(msg, 'DeprecationWarning', deprecated);
      }
    }
    return fn.apply(this, arguments);
  }

  // The wrapper will keep the same prototype as fn to maintain prototype chain
  Object.setPrototypeOf(deprecated, fn);
  if (fn.prototype) {
    // Setting this (rather than using Object.setPrototype, as above) ensures
    // that calling the unwrapped constructor gives an instanceof the wrapped
    // constructor.
    deprecated.prototype = fn.prototype;
  }

  return deprecated;
}

// FIXME: use Symbol on implementation done
var customPromisifySymbol = 'util:promisify:custom';
promisify.custom = customPromisifySymbol;
function promisify(original) {
  if (typeof original != 'function') {
    throw new TypeError('expect a function on promisify');
  }
  if (typeof original[customPromisifySymbol] == 'function') {
    return original[customPromisifySymbol];
  }
  return function promisified() {
    var args = Array.prototype.slice.call(arguments, 0);

    return new Promise((resolve, reject) => {
      args.push(callback);
      original.apply(this, args);

      function callback(err, result) {
        if (err != null) {
          reject(err);
          return;
        }
        resolve(result);
      }
    });
  };
}


exports.isNull = isNull;
exports.isUndefined = isUndefined;
exports.isNullOrUndefined = isNullOrUndefined;
exports.isNumber = isNumber;
exports.isBoolean = isBoolean;
exports.isString = isString;
exports.isObject = isObject;
exports.isFinite = isFinite;
exports.isFunction = isFunction;
exports.isBuffer = isBuffer;
exports.isRegExp = isRegExp;
exports.isArray = Array.isArray;
exports.exceptionWithHostPort = exceptionWithHostPort;
exports.errnoException = errnoException;
exports.stringToNumber = stringToNumber;
exports.inherits = inherits;
exports.format = format;
exports.formatValue = formatValue;
exports.deprecate = deprecate;
exports.promisify = promisify;
exports.isDate = isDate;
exports.isError = isError;
