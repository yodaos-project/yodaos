// Copyright (c) 2014 Taylor Hakes
// Copyright (c) 2014 Forbes Lindesay

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

/*
 * Forked from https://github.com/taylorhakes/promise-polyfill
 * Version 487e452.
 */

/*eslint-disable */

var STATE_PENDING = 0;
var STATE_FULFILLED = 1;
var STATE_REJECTED = 2;
var STATE_NEXT = 3;

// freeze the following static for performance
var emptyPromise = freezePromiseOnResolve(undefined);
var emptyArrayPromise = freezePromiseOnResolve([]);
var nullPromise = freezePromiseOnResolve(null);
var truePromise = freezePromiseOnResolve(true);
var falsePromise = freezePromiseOnResolve(false);

function freezePromiseOnResolve(value) {
  var self = new Promise(noop);
  resolve(self, value);
  self._handled = true;
  return Object.freeze(self);
}

function noop() {
  // for then
}

function Promise(fn) {
  if (!(this instanceof Promise))
    throw new TypeError('undefined is not a promise');
  if (typeof fn !== 'function')
    throw new TypeError('Promise resolver undefined is not a function');

  this._state = STATE_PENDING;
  this._handled = false;
  this._value = undefined;
  this._deferreds = [];

  if (fn === noop) {
    return;
  }
  doResolve(fn, this);
}

function handle(self, deferred) {
  while (self._state === STATE_NEXT) {
    self = self._value;
  }
  if (self._state === STATE_PENDING) {
    self._deferreds.push(deferred);
    return;
  }
  self._handled = true;
  process.nextTick(function() {
    var isFulfilled = self._state === STATE_FULFILLED;
    var cb = isFulfilled ? deferred.onFulfilled : deferred.onRejected;
    if (cb === null) {
      (isFulfilled ? resolve : reject)(deferred.promise, self._value);
      return;
    }
    var ret;
    try {
      ret = cb(self._value);
    } catch (e) {
      reject(deferred.promise, e);
      return;
    }
    resolve(deferred.promise, ret);
  });
}

function resolve(self, newValue) {
  try {
    // Promise Resolution Procedure:
    // https://github.com/promises-aplus/promises-spec#the-promise-resolution-procedure
    if (newValue === self)
      throw new TypeError('A promise cannot be resolved with itself.');

    if (
      newValue &&
      (typeof newValue === 'object' || typeof newValue === 'function')
    ) {
      if (newValue instanceof Promise) {
        self._state = STATE_NEXT;
        self._value = newValue;
        finale(self);
        return;
      } else if (typeof newValue.then === 'function') {
        doResolve(newValue.then.bind(newValue), self);
        return;
      }
    }
    self._state = STATE_FULFILLED;
    self._value = newValue;
    finale(self);
  } catch (e) {
    reject(self, e);
  }
}

function reject(self, newValue) {
  self._state = STATE_REJECTED;
  self._value = newValue;
  finale(self);
}

function finale(self) {
  if (self._state === STATE_REJECTED && self._deferreds.length === 0) {
    process.nextTick(function() {
      if (!self._handled) {
        Promise._unhandledRejectionFn(self._value, self);
      }
    });
  }

  for (var i = 0, len = self._deferreds.length; i < len; i++) {
    handle(self, self._deferreds[i]);
  }
  self._deferreds = null;
}

/**
 * Take a potentially misbehaving resolver function and make sure
 * onFulfilled and onRejected are only called once.
 *
 * Makes no guarantees about asynchrony.
 */
function doResolve(fn, self) {
  var done = false;
  try {
    fn(
      function(value) {
        if (done) return;
        done = true;
        resolve(self, value);
      },
      function(reason) {
        if (done) return;
        done = true;
        reject(self, reason);
      }
    );
  } catch (ex) {
    if (done) return;
    done = true;
    reject(self, ex);
  }
}

Promise.prototype.catch = function(onRejected) {
  return this.then(null, onRejected);
};

Promise.prototype.then = function(onFulfilled, onRejected) {
  // .then() is always needed to be a new instance.
  var prom = new this.constructor(noop);
  handle(this, {
    onFulfilled: typeof onFulfilled === 'function' ? onFulfilled : null,
    onRejected: typeof onRejected === 'function' ? onRejected : null,
    promise: prom,
  });
  return prom;
};

Promise.prototype.finally = function(callback) {
  var constructor = this.constructor;
  return this.then(
    function(value) {
      return constructor.resolve(callback()).then(function() {
        return value;
      });
    },
    function(reason) {
      return constructor.resolve(callback()).then(function() {
        return constructor.reject(reason);
      });
    }
  );
};

Promise.all = function(arr) {
  var args = arr;
  if (args.length === 0) {
    return Promise.resolve([]);
  }
  if (args.length === 1) {
    return Promise.resolve(arr[0]).then(function(data) {
      return [data];
    });
  }

  return new Promise(function(resolve, reject) {
    var remaining = args.length;

    function res(i, val) {
      try {
        if (val && (typeof val === 'object' || typeof val === 'function')) {
          if (typeof val.then === 'function') {
            val.then(
              function(val) {
                res(i, val);
              },
              reject
            );
            return;
          }
        }
        args[i] = val;
        if (--remaining === 0) {
          resolve(args);
        }
      } catch (ex) {
        reject(ex);
      }
    }

    for (var i = 0; i < args.length; i++) {
      res(i, args[i]);
    }
  });
};

Promise.resolve = function(value) {
  // detect the value and returns the freezed promise directly
  // only works for: `undefined`, `null`, `true`, `false` and array(0).
  if (value === undefined) {
    return emptyPromise;
  } else if (value === null) {
    return nullPromise;
  } else if (value === true) {
    return truePromise;
  } else if (value === false) {
    return falsePromise;
  } else if (Array.isArray(value) && value.length === 0) {
    return emptyArrayPromise;
  }

  if (value && typeof value === 'object' && value.constructor === Promise) {
    return value;
  }

  // Promise.resolve asserts no error handling is required, just call
  // resolve reduces the function calls.
  var self = new Promise(noop);
  resolve(self, value);
  return self;
};

Promise.reject = function(value) {
  // Promise.resolve asserts no error handling is required, just call
  // resolve reduces the function calls.
  var self = new Promise(noop);
  reject(self, value);
  return self;
};

Promise.race = function(values) {
  return new Promise(function(resolve, reject) {
    for (var i = 0, len = values.length; i < len; i++) {
      values[i].then(resolve, reject);
    }
  });
};

// Use polyfill for setImmediate for performance gains
Promise._immediateFn = function _immediateFn(fn) {
  setTimeout(fn, 0);
};

Promise._unhandledRejectionFn = function _unhandledRejectionFn(err, promise) {
  if (process.listeners('unhandledRejection').length === 0) {
    console.error('UnhandledPromiseRejection:', err.stack);
    return process.exit(1);
  }
  process.emit('unhandledRejection', err, promise);
};

module.exports = Promise;
