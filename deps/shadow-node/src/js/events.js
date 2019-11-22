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


function EventEmitter() {
  EventEmitter.init.call(this);
}

module.exports = EventEmitter;
module.exports.EventEmitter = EventEmitter;

EventEmitter.prototype._eventsCount = 0;
EventEmitter.prototype._maxListeners = undefined;

var defaultMaxListeners = 7;

Object.defineProperty(EventEmitter, 'defaultMaxListeners', {
  enumerable: true,
  get: function() {
    return defaultMaxListeners;
  },
  set: function(arg) {
    if (!arg || typeof arg !== 'number' || arg < 0) {
      throw new TypeError('defaultMaxListeners must be a non-negative number');
    }
    defaultMaxListeners = arg;
  }
});

EventEmitter.init = function() {
  if (this._events === undefined ||
      this._events === Object.getPrototypeOf(this)._events) {
    this._events = Object.create(null);
    this._eventsCount = 0;
  }

  this._maxListeners = this._maxListeners || undefined;
};


EventEmitter.prototype.emit = function(type) {
  if (!this._events) {
    this._events = Object.create(null);
  }

  // About to emit 'error' event but there are no listeners for it.
  if (type === 'error' && !this._events.error) {
    var err = arguments[1];
    if (err instanceof Error) {
      throw err;
    } else {
      throw new Error('Uncaught \'error\' event');
    }
  }

  var listeners = this._events[type];
  if (util.isArray(listeners)) {
    listeners = listeners.slice();
    var args = Array.prototype.slice.call(arguments, 1);
    for (var i = 0; i < listeners.length; ++i) {
      listeners[i].apply(this, args);
    }
    return true;
  }

  return false;
};


EventEmitter.prototype.addListener = function(type, listener) {
  var max, existing;

  if (!util.isFunction(listener)) {
    throw new TypeError('listener must be a function');
  }

  if (!this._events) {
    this._events = Object.create(null);
  }

  if (this._events.newListener !== undefined) {
    this.emit('newListener', type, listener);
  }

  existing = this._events[type];
  if (!existing) {
    existing = this._events[type] = [];
  }

  existing.push(listener);

  // check for listener leak
  max = $getMaxListeners(this);
  if (max > 0 && existing.length > max && !existing.warned) {
    existing.warned = true;
    var w = new Error('Possible EventEmitter memory leak detected. ' +
                  `${existing.length} or more \'${String(type)}\' listeners ` +
                  'added. Use emitter.setMaxListeners() to ' +
                  'increase limit');
    w.name = 'MaxListenersExceededWarning';
    w.emitter = this;
    w.type = type;
    w.count = existing.length;
    process.emitWarning(w);
    // TODO: print this warn.
  }

  return this;
};


EventEmitter.prototype.on = EventEmitter.prototype.addListener;


EventEmitter.prototype.once = function(type, listener) {
  if (!util.isFunction(listener)) {
    throw new TypeError('listener must be a function');
  }

  // eslint-disable-next-line func-style
  var f = function() {
    // here `this` is this not global, because EventEmitter binds event object
    // for this when it calls back the handler.
    this.removeListener(f.type, f);
    f.listener.apply(this, arguments);
  };

  f.type = type;
  f.listener = listener;

  this.on(type, f);

  return this;
};


EventEmitter.prototype.removeListener = function(type, listener) {
  if (!util.isFunction(listener)) {
    throw new TypeError('listener must be a function');
  }

  var list = this._events[type];
  if (Array.isArray(list)) {
    for (var i = list.length - 1; i >= 0; --i) {
      if (list[i] === listener ||
          (list[i].listener && list[i].listener === listener)) {
        list.splice(i, 1);
        if (!list.length) {
          delete this._events[type];
          if (this._events.removeListener !== undefined) {
            this.emit('removeListener', type, listener);
          }
        }
        break;
      }
    }
  }

  return this;
};


EventEmitter.prototype.removeAllListeners = function(type) {
  if (arguments.length === 0) {
    this._events = Object.create(null);
  } else {
    delete this._events[type];
  }

  return this;
};


EventEmitter.prototype.listeners = function(type) {
  var events = this._events;
  if (events === undefined)
    return [];

  var evlistener = events[type];
  if (evlistener === undefined)
    return [];

  if (typeof evlistener === 'function')
    return [evlistener.listener || evlistener];

  return unwrapListeners(evlistener);
};


EventEmitter.prototype.setMaxListeners = function(n) {
  if (!n || typeof n !== 'number' || n < 0) {
    throw new TypeError('arguments of setMaxListeners must be a ' +
    'non-negative number');
  }
  this._maxListeners = n;
  return this;
};

function $getMaxListeners(that) {
  if (that._maxListeners === undefined) {
    return EventEmitter.defaultMaxListeners;
  }
  return that._maxListeners;
}

EventEmitter.prototype.getMaxListeners = function() {
  return $getMaxListeners(this);
};


function unwrapListeners(arr) {
  var ret = new Array(arr.length);
  for (var i = 0; i < ret.length; ++i) {
    ret[i] = arr[i].listener || arr[i];
  }
  return ret;
}
